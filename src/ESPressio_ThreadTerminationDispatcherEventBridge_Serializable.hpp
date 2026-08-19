#pragma once

#include <ESPressio_IThreadTerminationDispatcherObserver.hpp>
#include <ESPressio_ThreadTerminationDispatcher.hpp>

#include "ESPressio_ThreadEvents_Serializable.hpp"

namespace ESPressio {
namespace Event {

class SerializableThreadTerminationDispatcherEventBridge final :
    public Threads::IThreadTerminationDispatcherObserver {

private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    SerializableThreadTerminationDispatcherEventBridge() = default;

public:
    static SerializableThreadTerminationDispatcherEventBridge& GetInstance() {
        static SerializableThreadTerminationDispatcherEventBridge instance;
        return instance;
    }

    bool Initialize() {
        if (_initialized) return true;
        _observerHandle =
            Threads::ThreadTerminationDispatcher::GetInstance()->
                RegisterObserver(this);
        _initialized = static_cast<bool>(_observerHandle);
        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const { return _initialized; }

    void OnThreadTerminationDispatcherInitialized(bool available) override {
        (new SerializableThreadTerminationDispatcherInitializedEvent(available))->Queue();
    }

    void OnThreadTerminationDispatchQueued(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new SerializableThreadTerminationDispatchQueuedEvent(snapshot))->Queue();
    }

    void OnThreadTerminationDispatchQueueFailed(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new SerializableThreadTerminationDispatchQueueFailedEvent(snapshot))->Queue();
    }

    void OnThreadTerminationDispatchStarted(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new SerializableThreadTerminationDispatchStartedEvent(snapshot))->Queue();
    }

    void OnThreadTerminationDispatchCompleted(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new SerializableThreadTerminationDispatchCompletedEvent(snapshot))->Queue();
    }
};

}
}
