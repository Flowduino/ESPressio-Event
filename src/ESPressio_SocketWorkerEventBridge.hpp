#pragma once

#include <ESPressio_ISocketWorkerObserver.hpp>
#include <ESPressio_SocketWorker.hpp>

#include "ESPressio_SocketWorkerEvents.hpp"

namespace ESPressio::Event {

class SocketWorkerEventBridge final :
    public Sockets::ISocketWorkerObserver {
private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

public:
    SocketWorkerEventBridge() = default;
    SocketWorkerEventBridge(const SocketWorkerEventBridge&) = delete;
    SocketWorkerEventBridge& operator=(const SocketWorkerEventBridge&) = delete;

    bool Initialize(Sockets::SocketWorker& worker) {
        if (_initialized) return true;
        _observerHandle = worker.RegisterObserver(this);
        _initialized = static_cast<bool>(_observerHandle);
        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const { return _initialized; }

    void OnSocketWorkerStarted(const char* name) override {
        (new SocketWorkerStartedEvent(name))->Queue();
    }

    void OnSocketWorkerStartFailed(const char* name) override {
        (new SocketWorkerStartFailedEvent(name))->Queue();
    }

    void OnSocketWorkerStopped() override {
        (new SocketWorkerStoppedEvent())->Queue();
    }
};

} // namespace ESPressio::Event
