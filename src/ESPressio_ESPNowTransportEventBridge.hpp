#pragma once

#include <ESPressio_ESPNowTransport.hpp>
#include <ESPressio_IESPNowTransportObserver.hpp>

#include "ESPressio_ESPNowEvents.hpp"

namespace ESPressio::Event {

class ESPNowTransportEventBridge final :
    public ESPNow::IESPNowTransportObserver {
private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    ESPNowTransportEventBridge() = default;

public:
    ESPNowTransportEventBridge(const ESPNowTransportEventBridge&) = delete;
    ESPNowTransportEventBridge& operator=(const ESPNowTransportEventBridge&) = delete;

    static ESPNowTransportEventBridge& GetInstance() {
        static ESPNowTransportEventBridge instance;
        return instance;
    }

    bool Initialize(
        ESPNow::ESPNowTransport& transport = ESPNow::ESPNowTransport::GetInstance()
    ) {
        if (_initialized) return true;
        _observerHandle = transport.RegisterObserver(this);
        _initialized = static_cast<bool>(_observerHandle);
        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const { return _initialized; }

    void OnESPNowTransportInitialized() override {
        (new ESPNowTransportInitializedEvent())->Queue();
    }

    void OnESPNowTransportInitializationFailed() override {
        (new ESPNowTransportInitializationFailedEvent())->Queue();
    }

    void OnESPNowTransportShutdown() override {
        (new ESPNowTransportShutdownEvent())->Queue();
    }

    void OnESPNowPeerAdded(const ESPNow::MacAddress& address) override {
        (new ESPNowPeerAddedEvent(address))->Queue();
    }

    void OnESPNowPeerRemoved(const ESPNow::MacAddress& address) override {
        (new ESPNowPeerRemovedEvent(address))->Queue();
    }

    void OnESPNowSendAccepted(
        const ESPNow::MacAddress& destination,
        uint8_t protocol,
        std::size_t payloadLength
    ) override {
        (new ESPNowSendAcceptedEvent(destination, protocol, payloadLength))->Queue();
    }

    void OnESPNowSendFailed(
        const ESPNow::MacAddress& destination,
        uint8_t protocol,
        std::size_t payloadLength
    ) override {
        (new ESPNowSendFailedEvent(destination, protocol, payloadLength))->Queue();
    }
};

} // namespace ESPressio::Event
