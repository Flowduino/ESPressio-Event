#pragma once

#if !__has_include(<ESPressio_Serializable.hpp>) || !__has_include(<ESPressio_BinaryArchive.hpp>)
#error "ESPressio Event Transport requires ESPressio-Serializable >= 0.9.0 in the consuming project."
#endif

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <ESPressio_BinaryArchive.hpp>
#include <ESPressio_SerializationTraits.hpp>
#include <ESPressio_Thread.hpp>

#include "ESPressio_EventManager.hpp"
#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_IEventManagerObserver.hpp"
#include "ESPressio_IEventTransport.hpp"
#include "ESPressio_EventTransportManagerObservable.hpp"

namespace ESPressio::Event {

class EventTransportManager final :
    public Threads::Thread,
    public IEventTransportReceiver,
    public IEventManagerObserver {

private:
    struct Registration {
        uint64_t TypeID = 0;
        uint32_t SchemaVersion = 1;
        std::type_index RuntimeType = std::type_index(typeid(void));
        EventTransportDirection Direction = EventTransportDirection::None;
        std::function<bool(IEvent*, std::vector<uint8_t>&)> Serialize;
        std::function<IEvent*(const uint8_t*, std::size_t)> Deserialize;
    };

    struct OutboundWork {
        IEvent* Event = nullptr;
        uint64_t TypeID = 0;
        Registration RegistrationSnapshot;
        EventDispatchMethod Method = EventDispatchMethod::Queue;
        EventPriority Priority = EventPriority::Normal;
        uint64_t MessageID = 0;
    };

    struct InboundWork {
        IEventTransport* Transport = nullptr;
        uint64_t TypeID = 0;
        Registration RegistrationSnapshot;
        std::vector<uint8_t> Packet;
    };

    mutable std::mutex _mutex;
    std::unordered_map<uint64_t, Registration> _registrations;
    std::unordered_map<std::type_index, uint64_t> _runtimeTypes;
    std::vector<IEventTransport*> _transports;
    std::deque<OutboundWork> _outbound;
    std::deque<InboundWork> _inbound;
    SemaphoreHandle_t _semaphore = nullptr;
    Observable::ObserverHandlePtr _eventManagerObserverHandle;
    std::shared_ptr<EventTransportManagerObservable> _observable =
        CreateEventTransportManagerObservable();
    std::atomic<uint64_t> _nextMessageID{1};
    bool _initialized = false;

    EventTransportManager() : Threads::Thread(false) {
        _semaphore = xSemaphoreCreateBinary();
    }

    static bool ParseEnvelope(
        const uint8_t* data,
        std::size_t size,
        EventTransportEnvelope& envelope,
        const uint8_t*& payload
    ) {
        if (data == nullptr || size < sizeof(EventTransportEnvelope)) return false;
        std::memcpy(&envelope, data, sizeof(envelope));
        if (envelope.Magic != EventTransportEnvelope::MagicValue ||
            envelope.Version != EventTransportEnvelope::CurrentVersion) return false;
        if (envelope.PayloadLength != size - sizeof(EventTransportEnvelope)) return false;
        payload = data + sizeof(EventTransportEnvelope);
        return true;
    }

    static std::vector<uint8_t> BuildPacket(
        const EventTransportEnvelope& envelope,
        const std::vector<uint8_t>& payload
    ) {
        std::vector<uint8_t> result(sizeof(EventTransportEnvelope) + payload.size());
        std::memcpy(result.data(), &envelope, sizeof(envelope));
        if (!payload.empty()) {
            std::memcpy(result.data() + sizeof(envelope), payload.data(), payload.size());
        }
        return result;
    }

    void Wake() {
        if (_semaphore != nullptr) xSemaphoreGive(_semaphore);
    }

    void ReleaseOutbound(OutboundWork& work) noexcept {
        if (work.Event != nullptr) {
            work.Event->__unref();
            work.Event = nullptr;
        }
    }

    void ProcessOutbound(OutboundWork work) {
        std::vector<IEventTransport*> transports;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            transports = _transports;
        }

        std::vector<uint8_t> payload;
        if (!work.RegistrationSnapshot.Serialize ||
            !work.RegistrationSnapshot.Serialize(work.Event, payload)) {
            ReleaseOutbound(work);
            return;
        }

        EventTransportEnvelope envelope;
        envelope.EventTypeID = work.RegistrationSnapshot.TypeID;
        envelope.SchemaVersion = work.RegistrationSnapshot.SchemaVersion;
        envelope.MessageID = work.MessageID;
        envelope.DispatchMethod = static_cast<uint8_t>(work.Method);
        envelope.Priority = static_cast<uint8_t>(work.Priority);
        envelope.HopCount = 0;
        envelope.PayloadLength = static_cast<uint32_t>(payload.size());

        auto bytes = BuildPacket(envelope, payload);
        EventTransportPacket packet{bytes.data(), bytes.size(), work.MessageID};
        for (IEventTransport* transport : transports) {
            if (transport != nullptr) {
                const bool accepted = transport->Send(packet);
                _observable->Notify([&](IEventTransportManagerObserver* o) {
                    o->OnOutboundEventHandedToTransport(
                        work.TypeID, work.MessageID, transport, accepted
                    );
                });
            }
        }

        ReleaseOutbound(work);
    }

    void ProcessInbound(InboundWork work) {
        EventTransportEnvelope envelope;
        const uint8_t* payload = nullptr;
        if (!ParseEnvelope(work.Packet.data(), work.Packet.size(), envelope, payload)) return;

        const Registration& registration = work.RegistrationSnapshot;
        if (!registration.Deserialize) return;
        IEvent* event = registration.Deserialize(payload, envelope.PayloadLength);
        if (event == nullptr) {
            return;
        }
        _observable->Notify([&](IEventTransportManagerObserver* o) {
            o->OnInboundEventDeserialized(envelope.EventTypeID, envelope.MessageID);
        });

        EventDispatchContext context;
        context.Origin = EventOrigin::Remote;
        context.TransportMessageID = envelope.MessageID;
        context.HopCount = envelope.HopCount;
        event->__setDispatchContext(context);

        if (envelope.DispatchMethod > static_cast<uint8_t>(EventDispatchMethod::Queue) ||
            envelope.Priority > static_cast<uint8_t>(EventPriority::High)) {
            delete event;
            return;
        }
        const auto method = static_cast<EventDispatchMethod>(envelope.DispatchMethod);
        const auto priority = static_cast<EventPriority>(envelope.Priority);
        if (method == EventDispatchMethod::Stack) event->Stack(priority);
        else event->Queue(priority);

        _observable->Notify([&](IEventTransportManagerObserver* o) {
            o->OnInboundEventDispatched(envelope.EventTypeID, envelope.MessageID);
        });
    }

    void OnLoop() override {
        if (_semaphore != nullptr) xSemaphoreTake(_semaphore, portMAX_DELAY);
        for (;;) {
            OutboundWork outbound;
            InboundWork inbound;
            bool hasOutbound = false;
            bool hasInbound = false;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (!_inbound.empty()) {
                    inbound = std::move(_inbound.front());
                    _inbound.pop_front();
                    hasInbound = true;
                } else if (!_outbound.empty()) {
                    outbound = _outbound.front();
                    _outbound.pop_front();
                    hasOutbound = true;
                }
            }
            if (hasInbound) ProcessInbound(std::move(inbound));
            else if (hasOutbound) ProcessOutbound(outbound);
            else break;
        }
    }

    void DropPendingLocked(
        uint64_t typeID,
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options,
        std::vector<OutboundWork>& discardedOutbound,
        std::size_t& discardedInbound
    ) {
        if (HasDirection(direction, EventTransportDirection::Outbound) &&
            options.PendingOutbound == EventTransportPendingAction::Discard) {
            auto it = _outbound.begin();
            while (it != _outbound.end()) {
                if (it->TypeID == typeID) {
                    discardedOutbound.push_back(*it);
                    it = _outbound.erase(it);
                } else {
                    ++it;
                }
            }
        }
        if (HasDirection(direction, EventTransportDirection::Inbound) &&
            options.PendingInbound == EventTransportPendingAction::Discard) {
            const auto before = _inbound.size();
            _inbound.erase(std::remove_if(_inbound.begin(), _inbound.end(),
                [&](const InboundWork& work) { return work.TypeID == typeID; }), _inbound.end());
            discardedInbound += before - _inbound.size();
        }
    }

public:
    ~EventTransportManager() override {
        Shutdown();
        Threads::Thread::Shutdown();
        if (_semaphore != nullptr) {
            xSemaphoreGive(_semaphore);
            vSemaphoreDelete(_semaphore);
            _semaphore = nullptr;
        }
    }

    EventTransportManager(const EventTransportManager&) = delete;
    EventTransportManager& operator=(const EventTransportManager&) = delete;

    static EventTransportManager& GetInstance() {
        static EventTransportManager instance;
        return instance;
    }

    void Initialize() override {
        if (_initialized) return;
        _eventManagerObserverHandle = EventManager::GetInstance()->RegisterObserver(this);
        if (!_eventManagerObserverHandle) return;
        if (GetThreadState() == Threads::ThreadState::Uninitialized) {
            Threads::Thread::Initialize();
        }
        if (GetThreadState() != Threads::ThreadState::Started) {
            Start();
        }
        _initialized = true;
    }

    bool IsInitialized() const noexcept {
        return _initialized;
    }

    void Shutdown() {
        if (!_initialized) return;
        _eventManagerObserverHandle.reset();
        std::deque<OutboundWork> outbound;
        std::vector<IEventTransport*> transports;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            outbound.swap(_outbound);
            _inbound.clear();
            transports.swap(_transports);
            _initialized = false;
        }
        for (auto& work : outbound) ReleaseOutbound(work);
        for (auto* transport : transports) {
            if (transport != nullptr) transport->SetReceiver(nullptr);
        }
    }

    bool RegisterTransport(IEventTransport* transport) {
        if (transport == nullptr) return false;
        bool added = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (std::find(_transports.begin(), _transports.end(), transport) == _transports.end()) {
                _transports.push_back(transport);
                added = true;
            }
        }
        if (added) {
            transport->SetReceiver(this);
            _observable->Notify([&](IEventTransportManagerObserver* o){ o->OnEventTransportRegistered(transport); });
        }
        return true;
    }

    void UnregisterTransport(IEventTransport* transport) {
        if (transport == nullptr) return;
        bool removed = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            const auto oldSize = _transports.size();
            _transports.erase(std::remove(_transports.begin(), _transports.end(), transport), _transports.end());
            removed = _transports.size() != oldSize;
        }
        if (removed) {
            transport->SetReceiver(nullptr);
            _observable->Notify([&](IEventTransportManagerObserver* o){ o->OnEventTransportUnregistered(transport); });
        }
    }

    template<typename TEvent, bool RequireInboundFactory>
    EventTransportRegistrationResult RegisterEventImpl(
        EventTransportDirection direction
    ) {
        static_assert(std::is_base_of_v<IEvent, TEvent>,
            "Transported types must derive from ESPressio::Event::IEvent.");
        static_assert(Serializable::IsSerializable<TEvent>,
            "Transported Events must implement ESPressio Serializable.");
        static_assert(EventTransportTypeTraits<TEvent>::Name.size() != 0,
            "Transported Events require ESPRESSIO_EVENT_TRANSPORT_TYPE(Type, StableName).");
        if constexpr (RequireInboundFactory) {
            static_assert(std::is_default_constructible_v<TEvent>,
                "Inbound transported Events must be default constructible.");
        }

        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        Registration proposed;
        proposed.TypeID = typeID;
        proposed.SchemaVersion = TEvent::GetSchemaVersion();
        proposed.RuntimeType = std::type_index(typeid(TEvent));
        proposed.Direction = direction;
        proposed.Serialize = [](IEvent* event, std::vector<uint8_t>& bytes) {
            auto* typed = dynamic_cast<TEvent*>(event);
            if (typed == nullptr) return false;
            Serializable::BinaryArchive archive;
            typed->Serialize(archive);
            bytes = archive.GetData();
            return !bytes.empty();
        };

        if constexpr (std::is_default_constructible_v<TEvent>) {
            proposed.Deserialize = [](const uint8_t* data, std::size_t size) -> IEvent* {
                Serializable::BinaryArchive archive;
                if (!archive.Load(data, size)) return nullptr;
                auto event = std::make_unique<TEvent>();
                if (!event->Deserialize(archive)) return nullptr;
                return event.release();
            };
        }

        EventTransportRegistrationResult result;
        EventTransportDirection before = EventTransportDirection::None;
        EventTransportDirection after = EventTransportDirection::None;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto found = _registrations.find(typeID);
            if (found == _registrations.end()) {
                _registrations.emplace(typeID, proposed);
                _runtimeTypes[proposed.RuntimeType] = typeID;
                result = EventTransportRegistrationResult::Registered;
                after = direction;
            } else if (found->second.RuntimeType != proposed.RuntimeType) {
                return EventTransportRegistrationResult::TypeConflict;
            } else {
                before = found->second.Direction;
                const auto merged = before | direction;
                if (merged == before) return EventTransportRegistrationResult::AlreadyRegistered;
                found->second.Direction = merged;
                if (!found->second.Deserialize && proposed.Deserialize) found->second.Deserialize = proposed.Deserialize;
                _runtimeTypes[proposed.RuntimeType] = typeID;
                after = merged;
                result = EventTransportRegistrationResult::Updated;
            }
        }
        if (result == EventTransportRegistrationResult::Registered) {
            _observable->Notify([&](IEventTransportManagerObserver* o){ o->OnEventTransportTypeRegistered(typeID, after); });
        } else {
            _observable->Notify([&](IEventTransportManagerObserver* o){ o->OnEventTransportTypeRegistrationChanged(typeID, before, after); });
        }
        return result;
    }

    template<typename TEvent>
    EventTransportRegistrationResult RegisterEvent(
        EventTransportDirection direction
    ) {
        /* Generic runtime direction may include Inbound, so require a factory. */
        return RegisterEventImpl<TEvent, true>(direction);
    }

    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterEvents(EventTransportDirection direction) {
        EventTransportBulkOperationResult result;
        result.Requested = sizeof...(TEvents);
        ([&] {
            const auto r = RegisterEvent<TEvents>(direction);
            if (r == EventTransportRegistrationResult::AlreadyRegistered) ++result.Unchanged;
            else if (r == EventTransportRegistrationResult::TypeConflict) ++result.Failed;
            else ++result.Changed;
        }(), ...);
        return result;
    }

    template<typename TEvent> auto RegisterInboundEvent() { return RegisterEventImpl<TEvent, true>(EventTransportDirection::Inbound); }
    template<typename TEvent> auto RegisterOutboundEvent() { return RegisterEventImpl<TEvent, false>(EventTransportDirection::Outbound); }
    template<typename TEvent> auto RegisterBidirectionalEvent() { return RegisterEventImpl<TEvent, true>(EventTransportDirection::Bidirectional); }

    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterInboundEvents() {
        EventTransportBulkOperationResult r; r.Requested = sizeof...(TEvents);
        ([&]{ auto x=RegisterInboundEvent<TEvents>(); if (x==EventTransportRegistrationResult::AlreadyRegistered) ++r.Unchanged; else if (x==EventTransportRegistrationResult::TypeConflict) ++r.Failed; else ++r.Changed; }(), ...);
        return r;
    }
    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterOutboundEvents() {
        EventTransportBulkOperationResult r; r.Requested = sizeof...(TEvents);
        ([&]{ auto x=RegisterOutboundEvent<TEvents>(); if (x==EventTransportRegistrationResult::AlreadyRegistered) ++r.Unchanged; else if (x==EventTransportRegistrationResult::TypeConflict) ++r.Failed; else ++r.Changed; }(), ...);
        return r;
    }
    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterBidirectionalEvents() {
        EventTransportBulkOperationResult r; r.Requested = sizeof...(TEvents);
        ([&]{ auto x=RegisterBidirectionalEvent<TEvents>(); if (x==EventTransportRegistrationResult::AlreadyRegistered) ++r.Unchanged; else if (x==EventTransportRegistrationResult::TypeConflict) ++r.Failed; else ++r.Changed; }(), ...);
        return r;
    }

    template<typename TEvent>
    EventTransportUnregistrationResult UnregisterEvent(
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        static_assert(EventTransportTypeTraits<TEvent>::Name.size() != 0,
            "Unregistering a transported Event requires its stable transport type trait.");
        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        EventTransportDirection before = EventTransportDirection::None;
        EventTransportDirection after = EventTransportDirection::None;
        EventTransportUnregistrationResult result;
        std::vector<OutboundWork> discardedOutbound;
        std::size_t discardedInbound = 0;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto found = _registrations.find(typeID);
            if (found == _registrations.end()) return EventTransportUnregistrationResult::NotRegistered;
            before = found->second.Direction;
            DropPendingLocked(typeID, direction, options, discardedOutbound, discardedInbound);
            after = static_cast<EventTransportDirection>(
                static_cast<uint8_t>(before) & ~static_cast<uint8_t>(direction));
            if (after == EventTransportDirection::None) {
                _runtimeTypes.erase(found->second.RuntimeType);
                _registrations.erase(found);
                result = EventTransportUnregistrationResult::Removed;
            } else {
                found->second.Direction = after;
                result = EventTransportUnregistrationResult::Updated;
            }
        }
        for (auto& work : discardedOutbound) ReleaseOutbound(work);
        (void)discardedInbound;
        _observable->Notify([&](IEventTransportManagerObserver* o) {
            o->OnEventTransportTypeUnregistered(typeID, before, after);
        });
        return result;
    }

    template<typename... TEvents>
    EventTransportBulkOperationResult UnregisterEvents(
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        EventTransportBulkOperationResult result;
        result.Requested = sizeof...(TEvents);
        ([&] {
            const auto r = UnregisterEvent<TEvents>(direction, options);
            if (r == EventTransportUnregistrationResult::NotRegistered) ++result.Unchanged;
            else ++result.Changed;
        }(), ...);
        return result;
    }

    template<typename TEvent> auto UnregisterInboundEvent(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(EventTransportDirection::Inbound, o); }
    template<typename TEvent> auto UnregisterOutboundEvent(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(EventTransportDirection::Outbound, o); }
    template<typename TEvent> auto UnregisterBidirectionalEvent(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(EventTransportDirection::Bidirectional, o); }
    template<typename... TEvents> auto UnregisterInboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(EventTransportDirection::Inbound, o); }
    template<typename... TEvents> auto UnregisterOutboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(EventTransportDirection::Outbound, o); }
    template<typename... TEvents> auto UnregisterBidirectionalEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(EventTransportDirection::Bidirectional, o); }

    EventTransportBulkOperationResult UnregisterAllEvents(
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        EventTransportBulkOperationResult result;
        std::vector<OutboundWork> discardedOutbound;
        std::size_t discardedInbound = 0;
        std::vector<std::tuple<uint64_t, EventTransportDirection, EventTransportDirection>> changes;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            result.Requested = _registrations.size();
            std::vector<uint64_t> erase;
            for (auto& [typeID, registration] : _registrations) {
                if (!HasDirection(registration.Direction, direction) &&
                    direction != EventTransportDirection::Bidirectional) {
                    ++result.Unchanged;
                    continue;
                }
                const auto before = registration.Direction;
                DropPendingLocked(typeID, direction, options, discardedOutbound, discardedInbound);
                const auto remaining = static_cast<EventTransportDirection>(
                    static_cast<uint8_t>(registration.Direction) & ~static_cast<uint8_t>(direction));
                changes.emplace_back(typeID, before, remaining);
                if (remaining == EventTransportDirection::None) erase.push_back(typeID);
                else registration.Direction = remaining;
                ++result.Changed;
            }
            for (auto id : erase) {
                auto found = _registrations.find(id);
                if (found != _registrations.end()) {
                    _runtimeTypes.erase(found->second.RuntimeType);
                    _registrations.erase(found);
                }
            }
        }
        for (auto& work : discardedOutbound) ReleaseOutbound(work);
        (void)discardedInbound;
        for (const auto& [typeID, before, after] : changes) {
            _observable->Notify([&](IEventTransportManagerObserver* o) {
                o->OnEventTransportTypeUnregistered(typeID, before, after);
            });
        }
        return result;
    }

    EventTransportBulkOperationResult UnregisterAllInboundEvents(
        const EventTransportUnregistrationOptions& options = {}
    ) { return UnregisterAllEvents(EventTransportDirection::Inbound, options); }

    EventTransportBulkOperationResult UnregisterAllOutboundEvents(
        const EventTransportUnregistrationOptions& options = {}
    ) { return UnregisterAllEvents(EventTransportDirection::Outbound, options); }

    EventTransportBulkOperationResult UnregisterAllBidirectionalEvents(
        const EventTransportUnregistrationOptions& options = {}
    ) { return UnregisterAllEvents(EventTransportDirection::Bidirectional, options); }

    Observable::ObserverHandlePtr RegisterObserver(
        IEventTransportManagerObserver* observer
    ) {
        return _observable->RegisterObserver(observer);
    }

    void UnregisterObserver(
        IEventTransportManagerObserver* observer
    ) {
        _observable->UnregisterObserver(observer);
    }

    void OnEventDispatched(
        IEvent* event,
        EventDispatchMethod method,
        EventPriority priority,
        const EventDispatchContext& context
    ) override {
        if (!_initialized || event == nullptr || context.Origin != EventOrigin::Local) return;
        uint64_t typeID = 0;
        uint64_t messageID = 0;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto runtime = _runtimeTypes.find(std::type_index(typeid(*event)));
            if (runtime == _runtimeTypes.end()) return;
            auto registration = _registrations.find(runtime->second);
            if (registration == _registrations.end() ||
                !HasDirection(registration->second.Direction, EventTransportDirection::Outbound)) return;
            event->__ref();
            typeID = runtime->second;
            messageID = _nextMessageID.fetch_add(1);
            _outbound.push_back({
                event, typeID, registration->second, method, priority, messageID
            });
        }
        _observable->Notify([&](IEventTransportManagerObserver* o){ o->OnOutboundEventAccepted(typeID, messageID); });
        Wake();
    }

    void ReceiveEventTransportPacket(
        IEventTransport* transport,
        const uint8_t* data,
        std::size_t size
    ) override {
        if (!_initialized || data == nullptr || size < sizeof(EventTransportEnvelope)) return;
        EventTransportEnvelope envelope;
        const uint8_t* payload = nullptr;
        if (!ParseEnvelope(data, size, envelope, payload)) {
            _observable->Notify([&](IEventTransportManagerObserver* o){ o->OnInboundPacketRejected(0, 0, transport); });
            return;
        }
        (void)payload;
        bool accepted = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto found = _registrations.find(envelope.EventTypeID);
            if (found != _registrations.end() &&
                HasDirection(found->second.Direction, EventTransportDirection::Inbound)) {
                _inbound.push_back({
                    transport, envelope.EventTypeID, found->second,
                    std::vector<uint8_t>(data, data + size)
                });
                accepted = true;
            }
        }
        if (accepted) {
            _observable->Notify([&](IEventTransportManagerObserver* o){ o->OnInboundPacketAccepted(envelope.EventTypeID, envelope.MessageID, transport); });
            Wake();
        } else {
            _observable->Notify([&](IEventTransportManagerObserver* o){ o->OnInboundPacketRejected(envelope.EventTypeID, envelope.MessageID, transport); });
        }
    }
};

}
