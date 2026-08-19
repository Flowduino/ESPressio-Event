#pragma once

#include <ESPressio_IThreadTerminationDispatcherObserver.hpp>
#include <ESPressio_ThreadTerminationDispatcher.hpp>

#include "ESPressio_ThreadEvents.hpp"

namespace ESPressio {
namespace Event {

class ThreadTerminationDispatcherEventBridge final :
    public Threads::IThreadTerminationDispatcherObserver {

private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    ThreadTerminationDispatcherEventBridge() = default;

public:
    ThreadTerminationDispatcherEventBridge(
        const ThreadTerminationDispatcherEventBridge&
    ) = delete;

    ThreadTerminationDispatcherEventBridge& operator=(
        const ThreadTerminationDispatcherEventBridge&
    ) = delete;

    static ThreadTerminationDispatcherEventBridge& GetInstance() {
        static ThreadTerminationDispatcherEventBridge instance;
        return instance;
    }

    bool Initialize() {
        if (_initialized) {
            return true;
        }

        _observerHandle =
            Threads::ThreadTerminationDispatcher::
                GetInstance()->
                RegisterObserver(this);

        _initialized =
            static_cast<bool>(_observerHandle);

        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const {
        return _initialized;
    }

    void OnThreadTerminationDispatcherInitialized(
        bool available
    ) override {
        (new ThreadTerminationDispatcherInitializedEvent(available))->Queue();
    }

    void OnThreadTerminationDispatchQueued(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadTerminationDispatchQueuedEvent(snapshot))->Queue();
    }

    void OnThreadTerminationDispatchQueueFailed(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadTerminationDispatchQueueFailedEvent(snapshot))->Queue();
    }

    void OnThreadTerminationDispatchStarted(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadTerminationDispatchStartedEvent(snapshot))->Queue();
    }

    void OnThreadTerminationDispatchCompleted(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadTerminationDispatchCompletedEvent(snapshot))->Queue();
    }
};

}
}
