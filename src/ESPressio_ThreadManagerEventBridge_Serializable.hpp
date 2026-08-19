#pragma once

#include <cstdint>

#include <ESPressio_IThreadManagerObserver.hpp>
#include <ESPressio_ThreadManager.hpp>

#include "ESPressio_ThreadEvents_Serializable.hpp"
#include "thread-events/ESPressio_ThreadEventBridgeHelpers_Serializable.hpp"

namespace ESPressio {
namespace Event {

class SerializableThreadManagerEventBridge final :
    public Threads::IThreadManagerObserver {

private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    SerializableThreadManagerEventBridge() = default;

public:
    static SerializableThreadManagerEventBridge& GetInstance() {
        static SerializableThreadManagerEventBridge instance;
        return instance;
    }

    bool Initialize() {
        if (_initialized) return true;
        _observerHandle =
            Threads::ThreadManager::GetInstance()->
                RegisterObserver(this);
        _initialized = static_cast<bool>(_observerHandle);
        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const { return _initialized; }

    void OnThreadRegistered(
        Threads::IThread*,
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new SerializableThreadRegisteredEvent(snapshot))->Queue();
    }

    void OnThreadRegistrationFailed(
        Threads::IThread* thread,
        std::exception_ptr cause
    ) override {
        (new SerializableThreadRegistrationFailedEvent(
            static_cast<uint64_t>(
                reinterpret_cast<uintptr_t>(thread)
            ),
            Internal::DescribeThreadBridgeException(cause)
        ))->Queue();
    }

    void OnThreadRemoved(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new SerializableThreadRemovedEvent(snapshot))->Queue();
    }

    void OnThreadCleanupClaimed(
        Threads::IThread*,
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new SerializableThreadCleanupClaimedEvent(snapshot))->Queue();
    }

    void OnThreadCleanupDeferred(
        const Threads::ThreadManagerCleanupResult& result
    ) override {
        (new SerializableThreadCleanupDeferredEvent(result))->Queue();
    }

    void OnThreadCleanupStarted(
        const Threads::ThreadManagerCleanupResult& result
    ) override {
        (new SerializableThreadCleanupStartedEvent(result))->Queue();
    }

    void OnThreadCleanupCompleted(
        const Threads::ThreadManagerCleanupResult& result
    ) override {
        (new SerializableThreadCleanupCompletedEvent(result))->Queue();
    }

    void OnThreadCleanupFailed(
        const Threads::ThreadManagerCleanupResult& result,
        std::exception_ptr cause
    ) override {
        (new SerializableThreadCleanupFailedEvent(
            result,
            Internal::DescribeThreadBridgeException(cause)
        ))->Queue();
    }

    void OnThreadManagerInitializationCompleted(
        const Threads::ThreadManagerInitializationResult& result
    ) override {
        (new SerializableThreadManagerInitializationCompletedEvent(result))->Queue();
    }
};

}
}
