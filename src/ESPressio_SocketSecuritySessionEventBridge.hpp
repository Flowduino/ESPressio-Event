#pragma once

#include <ESPressio_ISocketSecuritySessionObserver.hpp>
#include <ESPressio_SocketSecuritySession.hpp>

#include "ESPressio_SocketEvents.hpp"

namespace ESPressio::Event {

class SocketSecuritySessionEventBridge final :
    public Sockets::ISocketSecuritySessionObserver {
private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

public:
    SocketSecuritySessionEventBridge() = default;
    SocketSecuritySessionEventBridge(const SocketSecuritySessionEventBridge&) = delete;
    SocketSecuritySessionEventBridge& operator=(const SocketSecuritySessionEventBridge&) = delete;

    bool Initialize(Sockets::SocketSecuritySession& session) {
        if (_initialized) return true;
        _observerHandle = session.RegisterObserver(this);
        _initialized = static_cast<bool>(_observerHandle);
        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const { return _initialized; }

    void OnSocketSecuritySessionFaulted(const Security::SecurityResult& result) override {
        (new SocketSecuritySessionFaultedEvent(result))->Queue();
    }

    void OnSocketSecuritySessionReset() override {
        (new SocketSecuritySessionResetEvent())->Queue();
    }
};

} // namespace ESPressio::Event
