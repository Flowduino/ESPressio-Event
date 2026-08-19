#pragma once

#include <cstdint>

#include <ESPressio_IThreadManagerObserver.hpp>
#include <ESPressio_ThreadManager.hpp>

#include "ESPressio_ThreadEvents.hpp"

namespace ESPressio {
namespace Event {

class ThreadManagerEventBridge final :
    public Threads::IThreadManagerObserver {

private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    ThreadManagerEventBridge() = default;

public:
    ThreadManagerEventBridge(const ThreadManagerEventBridge&) = delete;
    ThreadManagerEventBridge& operator=(const ThreadManagerEventBridge&) = delete;

    static ThreadManagerEventBridge& GetInstance() {
        static ThreadManagerEventBridge instance;
        return instance;
    }

    bool Initialize() {
        if (_initialized) {
            return true;
        }

        _observerHandle =
            Threads::ThreadManager::
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

    void OnThreadRegistered(
        Threads::IThread*,
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadRegisteredEvent(snapshot))->Queue();
    }

    void OnThreadRegistrationFailed(
        Threads::IThread* thread,
        std::exception_ptr cause
    ) override {
        (new ThreadRegistrationFailedEvent(
            reinterpret_cast<uintptr_t>(thread),
            cause
        ))->Queue();
    }

    void OnThreadRemoved(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadRemovedEvent(snapshot))->Queue();
    }

    void OnThreadCleanupClaimed(
        Threads::IThread*,
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadCleanupClaimedEvent(snapshot))->Queue();
    }

    void OnThreadCleanupDeferred(
        const Threads::ThreadManagerCleanupResult& result
    ) override {
        (new ThreadCleanupDeferredEvent(result))->Queue();
    }

    void OnThreadCleanupStarted(
        const Threads::ThreadManagerCleanupResult& result
    ) override {
        (new ThreadCleanupStartedEvent(result))->Queue();
    }

    void OnThreadCleanupCompleted(
        const Threads::ThreadManagerCleanupResult& result
    ) override {
        (new ThreadCleanupCompletedEvent(result))->Queue();
    }

    void OnThreadCleanupFailed(
        const Threads::ThreadManagerCleanupResult& result,
        std::exception_ptr cause
    ) override {
        (new ThreadCleanupFailedEvent(result, cause))->Queue();
    }

    void OnThreadManagerInitializationCompleted(
        const Threads::ThreadManagerInitializationResult& result
    ) override {
        (new ThreadManagerInitializationCompletedEvent(result))->Queue();
    }
};

}
}
