#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <shared_mutex>
#include <vector>

#include <ESPressio_ThreadSafe.hpp>
#include <ESPressio_IObservable.hpp>
#include <ESPressio_Clock.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventObserver.hpp"


namespace ESPressio {

    namespace Event {

        class IEventListenerHandle {
            public:
                virtual ~IEventListenerHandle() { };
                virtual void Unregister() = 0;
                virtual bool IsRegistered() const = 0;
        };

        /*
            `IEventListener` is an Interface which you can implement to listen for Events.
            You can register a Listener for a specific Event Type and when that Event is Dispatched, your Listener will be called.
        */
        class IEventListener {
            public:
                virtual ~IEventListener() { }

                virtual IEventListenerHandle* RegisterListener(
                    std::type_index eventType,
                    std::function<void(
                        IEvent*,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority
                    )> callback,
                    EventListenerInterest interest = EventListenerInterest::All,
                    EventTime maximumTimeSinceDispatch = EventTime(0),
                    std::function<bool(IEvent*)> customInterestCallback = nullptr
                ) = 0;

                template <typename EventType>
                IEventListenerHandle* RegisterListener(
                    std::function<void(
                        IEvent*,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority)> callback,
                        EventListenerInterest interest = EventListenerInterest::All,
                        EventTime maximumTimeSinceDispatch = EventTime(0),
                        std::function<bool(IEvent*)> customInterestCallback = nullptr
                );

                /// Registers a typed Observer using the same asynchronous Event
                /// pipeline and lifetime handle as callback-based listeners.
                template <typename EventType>
                IEventListenerHandle* RegisterObserver(
                    IEventObserver<EventType>* observer,
                    EventListenerInterest interest = EventListenerInterest::All,
                    EventTime maximumTimeSinceDispatch = EventTime(0)
                ) {
                    if (observer == nullptr) {
                        throw Observable::InvalidObserverRegistrationException();
                    }

                    std::function<bool(IEvent*)> customInterestCallback = nullptr;
                    if (interest == EventListenerInterest::Custom) {
                        customInterestCallback = [observer](IEvent* event) {
                            EventType* typedEvent = dynamic_cast<EventType*>(event);
                            return typedEvent != nullptr &&
                                observer->IsInterestedInEvent(typedEvent);
                        };
                    }

                    return RegisterListener(
                        std::type_index(typeid(EventType)),
                        [observer](
                            IEvent* event,
                            EventDispatchMethod dispatchMethod,
                            EventPriority priority) {
                            EventType* typedEvent = dynamic_cast<EventType*>(event);
                            if (typedEvent != nullptr) {
                                observer->OnEvent(
                                    typedEvent, dispatchMethod, priority);
                            }
                        },
                        interest,
                        maximumTimeSinceDispatch,
                        customInterestCallback
                    );
                }

                virtual void UnregisterListener(std::type_index eventType, IEventListenerHandle* handler) = 0;

                template <typename EventType>
                void UnregisterListener(IEventListenerHandle* handler);
        };

        /*
            `EventListenerHandle` is returned when invoking `RegisterListener` against any implementor of `IEventListener`.
            This class is used to manage the lifetime of the Listener and to unregister the Listener when it is no longer needed.
            You should retain your reference to this Handler and call `Unregister` against it when you are done with the Listener (and on your objects' Destructor if applicable).
            DON'T FORGET: YOUR code takes ownership of the EventListenerHandle, and you must destroy (`delete`) it when you are done with it.
        */
        class EventListenerHandle : public IEventListenerHandle {
            private:
                mutable ReadWriteMutex<bool> _isRegistered = ReadWriteMutex<bool>(true);
                std::type_index _eventType; // This is the Event Type (Hash) which we can use to quickly look up the Listeners for this Event Type
                IEventListener* _listener; // This is a Weak Reference to the Listener (it will be nullified automatically when the Event Listener is destroyed)
            public:
            // Constructor/Deconstructor

                EventListenerHandle(std::type_index eventType, IEventListener* listener) : _eventType(eventType), _listener(listener) { }

                template <typename EventType>
                EventListenerHandle(IEventListener* listener) : _eventType(typeid(EventType)), _listener(listener) { }
                
                ~EventListenerHandle() override {
                    Unregister();
                }

            // Methods
                // Will safely `Unregister` the Listener
                void Unregister() override {
                    if (!_isRegistered.Get() || _listener == nullptr) { return; } // If the Listener is already Unregistered, or the Listener is no longer alive, then we don't need to do anything
                    _listener->UnregisterListener(_eventType, this);
                    _isRegistered.Set(false);
                }

            // Getters
                
                bool IsRegistered() const override { return _isRegistered.Get(); }

                void ForceUnregister() {
                    _isRegistered.Set(false);
                    _listener = nullptr;
                }
        };

        class EventListener : public IEventListener {
            private:
            
                /// This is an ugly hack to work around a limitation in the C++ language
                /// Basically, Vectors cannot contain template types, so we need a common base class for the Vector instead.
                /// All templated methods are excluded from the Interface.
                class IEventListenerContainer {
                    public:
                        virtual ~IEventListenerContainer() { }
                        virtual IEventListenerHandle* GetListenerHandler() const = 0;
                        virtual IEventListener* GetRequester() const = 0;
                        virtual EventListenerInterest GetInterest() const = 0;
                        virtual EventTime GetMaximumTimeSinceDispatch() const = 0;
                        virtual void ProcessEvent(
                            IEvent* event,
                            EventDispatchMethod dispatchMethod,
                            EventPriority priority
                        ) = 0;
                };

                /// `EventListenerContainer` is a class which holds all information about a specific Listener for a specific Event Type.
                template <typename EventType>
                class EventListenerContainer : public IEventListenerContainer {
                    private:
                        IEventListener* _requester; // We will use this to determine if the requester is still alive
                        std::function<void(EventType*, EventDispatchMethod dispatchMethod, EventPriority priority)> _callback;
                        IEventListenerHandle* _listenerHandler;
                        EventListenerInterest _interest = EventListenerInterest::All; // Default to All
                        EventTime _maximumTimeSinceDispatch;
                        std::function<bool(EventType*)> _customInterestCallback = nullptr; // Default to nullptr because we only use this Callback if the interest is Custom
                    public:
                    // Constructor

                        EventListenerContainer(
                            IEventListener* requester,
                            std::function<void(
                                EventType* eventType,
                                EventDispatchMethod dispatchMethod,
                                EventPriority priority
                            )> callback,
                            IEventListenerHandle* listenerHandler,
                            EventListenerInterest interest = EventListenerInterest::All,
                            EventTime maximumTimeSinceDispatch = EventTime(0),
                            std::function<bool(EventType*)> customInterestCallback = nullptr
                        ) : _requester(requester), _callback(callback), _listenerHandler(listenerHandler), _interest(interest), _maximumTimeSinceDispatch(maximumTimeSinceDispatch), _customInterestCallback(customInterestCallback) { }

                    // Destructor
                        
                        ~EventListenerContainer() override { 
                            // Do nothing, because the Container is owned by the Listener and it'll take responsibility for destroying it only when it's safe to do so!
                        }

                    // Getters

                        IEventListenerHandle* GetListenerHandler() const override { return _listenerHandler; }
                        IEventListener* GetRequester() const override { return _requester; }
                        inline std::function<void(EventType*, EventDispatchMethod dispatchMethod, EventPriority priority)> GetCallback() const { return _callback; }
                        EventListenerInterest GetInterest() const override { return _interest; }
                        EventTime GetMaximumTimeSinceDispatch() const override { return _maximumTimeSinceDispatch; }
                        std::function<bool(EventType*)> GetCustomInterestCallback() const { return _customInterestCallback; }

                        void ProcessEvent(
                            IEvent* event,
                            EventDispatchMethod dispatchMethod,
                            EventPriority priority) override {
                            EventType* typedEvent = dynamic_cast<EventType*>(event);
                            if (typedEvent == nullptr) { return; }

                            bool interested = _interest == EventListenerInterest::All;
                            if (_interest == EventListenerInterest::YoungerThan) {
                                interested = Timing::ClockBase::GetNanoseconds(
                                    event->GetTimeSinceDispatch()
                                ) < Timing::ClockBase::GetNanoseconds(
                                    _maximumTimeSinceDispatch
                                );
                            } else if (_interest == EventListenerInterest::Custom) {
                                interested = _customInterestCallback != nullptr &&
                                    _customInterestCallback(typedEvent);
                            }

                            if (interested) {
                                _callback(typedEvent, dispatchMethod, priority);
                            }
                        }

                    // Setters

                        void SetRequester(IEventListener* requester) { _requester = requester; }
                        void SetCallback(std::function<void(EventType*, EventDispatchMethod dispatchMethod, EventPriority priority)> callback) { _callback = callback; }
                        void SetInterest(EventListenerInterest interest) { _interest = interest; }
                        void SetMaximumTimeSinceDispatch(EventTime maximumTimeSinceDispatch) { _maximumTimeSinceDispatch = maximumTimeSinceDispatch; }
                        void SetCustomInterestCallback(std::function<bool(EventType*)> customInterestCallback) { _customInterestCallback = customInterestCallback; }
                };

                typedef std::vector<std::shared_ptr<IEventListenerContainer>> EventListeners;
                typedef std::shared_ptr<const EventListeners>
                    EventListenersSnapshot;
                typedef std::unordered_map<
                    std::type_index, EventListenersSnapshot
                > EventListenersMap;

                /// This is a map of Event Types to a collection of Listeners for that Event Type
                EventListenersMap _eventListeners;
                std::shared_mutex _eventListenersMutex;

                std::shared_ptr<EventListeners> CopyListenersForEventType(
                    std::type_index eventType
                ) {
                    const auto found = _eventListeners.find(eventType);
                    return found == _eventListeners.end() || !found->second
                        ? std::make_shared<EventListeners>()
                        : std::make_shared<EventListeners>(*found->second);
                }

            protected:

                inline virtual void OnListenerRegistered(std::type_index eventType) {
                    (void)eventType;
                }

                inline virtual void OnListenerUnregistered(std::type_index eventType) {
                    (void)eventType;
                }

                void UnregisterAllListeners() noexcept {
                    for (;;) {
                        std::type_index eventType(typeid(void));
                        bool found = false;
                        std::unique_lock<std::shared_mutex> lock(_eventListenersMutex);
                        if (_eventListeners.empty()) {
                            return;
                        }
                        const auto listenersForType =
                            _eventListeners.begin();
                        eventType = listenersForType->first;
                        found = true;
                        for (const auto& listener :
                            *listenersForType->second) {
                            static_cast<EventListenerHandle*>(
                                listener->GetListenerHandler()
                            )->ForceUnregister();
                        }
                        _eventListeners.erase(listenersForType);
                        lock.unlock();
                        if (found) {
                            try {
                                OnListenerUnregistered(eventType);
                            } catch (...) { }
                        }
                    }
                }

            public:
                virtual ~EventListener() {
                    UnregisterAllListeners();
                }

                IEventListenerHandle* RegisterListener(
                    std::type_index eventType,
                    std::function<void(
                        IEvent*,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority
                    )> callback,
                    EventListenerInterest interest = EventListenerInterest::All,
                    EventTime maximumTimeSinceDispatch = EventTime(0),
                    std::function<bool(IEvent*)> customInterestCallback = nullptr

                ) override {
                    std::unique_ptr<EventListenerHandle> handler(
                        new EventListenerHandle(eventType, this));
                    std::unique_lock<std::shared_mutex> lock(_eventListenersMutex);
                    std::shared_ptr<EventListeners> typeListeners =
                        CopyListenersForEventType(eventType);
                    const bool isFirstListener = typeListeners->empty();

                    std::shared_ptr<EventListenerContainer<IEvent>> listener =
                        std::make_shared<EventListenerContainer<IEvent>>(
                            this, callback, handler.get(), interest,
                            maximumTimeSinceDispatch, customInterestCallback);

                    typeListeners->push_back(listener);
                    _eventListeners[eventType] = typeListeners;

                    lock.unlock();
                    if (isFirstListener) { OnListenerRegistered(eventType); }
                    return handler.release();
                }

                template <typename EventType>
                IEventListenerHandle* RegisterListener(
                    std::function<void(
                        EventType*,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority)> callback,
                        EventListenerInterest interest = EventListenerInterest::All,
                        EventTime maximumTimeSinceDispatch = EventTime(0),
                        std::function<bool(EventType*)> customInterestCallback = nullptr

                ) {
                    std::type_index eventType = typeid(EventType);
                    std::unique_ptr<EventListenerHandle> handler(
                        new EventListenerHandle(eventType, this));
                    std::unique_lock<std::shared_mutex> lock(_eventListenersMutex);
                    std::shared_ptr<EventListeners> typeListeners =
                        CopyListenersForEventType(eventType);
                    const bool isFirstListener = typeListeners->empty();

                    std::shared_ptr<EventListenerContainer<EventType>> listener =
                        std::make_shared<EventListenerContainer<EventType>>(
                            this, callback, handler.get(), interest,
                            maximumTimeSinceDispatch, customInterestCallback);

                    typeListeners->push_back(listener);
                    _eventListeners[eventType] = typeListeners;

                    lock.unlock();
                    if (isFirstListener) { OnListenerRegistered(eventType); }
                    return handler.release();
                }

                void UnregisterListener(
                    std::type_index eventType,
                    IEventListenerHandle* handler) override {
                    std::unique_lock<std::shared_mutex> lock(_eventListenersMutex);
                    const auto listenersForType = _eventListeners.find(eventType);
                    if (listenersForType == _eventListeners.end() ||
                        listenersForType->second == nullptr) {
                        return;
                    }
                    std::shared_ptr<EventListeners> typeListeners =
                        std::make_shared<EventListeners>(
                            *listenersForType->second
                        );
                    bool removed = false;
                    for (auto it = typeListeners->begin(); it != typeListeners->end(); it++) {
                        if ((*it)->GetListenerHandler() == handler) {
                            static_cast<EventListenerHandle*>(handler)->ForceUnregister(); // Forcibly Unregister the Handle
                            typeListeners->erase(it);
                            removed = true;
                            break;
                        }
                    }
                    const bool removedLastListener = removed && typeListeners->empty();
                    if (removedLastListener) {
                        _eventListeners.erase(eventType);
                    } else if (removed) {
                        _eventListeners[eventType] = typeListeners;
                    }
                    lock.unlock();
                    if (removedLastListener) {
                        OnListenerUnregistered(eventType);
                    }
                }

                template <typename EventType>
                void UnregisterListener(IEventListenerHandle* handler) {
                    std::type_index eventType = typeid(EventType);
                    UnregisterListener(eventType, handler);
                }

                inline void ProcessEvent(IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) {
                    EventListenersSnapshot listeners;
                    {
                        std::shared_lock<std::shared_mutex> lock(_eventListenersMutex);
                        const auto listenersForType = _eventListeners.find(typeid(*event));
                        if (listenersForType == _eventListeners.end() ||
                            listenersForType->second == nullptr) {
                            return;
                        }
                        listeners = listenersForType->second;
                    }

                    for (const std::shared_ptr<IEventListenerContainer>& listener : *listeners) {
                        listener->ProcessEvent(event, dispatchMethod, priority);
                    }
                }
        };

    }

}
