#pragma once

#include <ESPressio_IThreadGarbageCollectorObserver.hpp>
#include <ESPressio_ThreadGarbageCollector.hpp>

#include "ESPressio_ThreadEvents_Serializable.hpp"
#include "thread-events/ESPressio_ThreadEventBridgeHelpers_Serializable.hpp"

namespace ESPressio {
namespace Event {

class SerializableThreadGarbageCollectorEventBridge final :
    public Threads::IThreadGarbageCollectorObserver {

private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    SerializableThreadGarbageCollectorEventBridge() = default;

public:
    static SerializableThreadGarbageCollectorEventBridge& GetInstance() {
        static SerializableThreadGarbageCollectorEventBridge instance;
        return instance;
    }

    bool Initialize() {
        if (_initialized) return true;
        _observerHandle =
            Threads::ThreadGarbageCollector::GetInstance()->
                RegisterObserver(this);
        _initialized = static_cast<bool>(_observerHandle);
        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const { return _initialized; }

    void OnThreadGarbageCollectorInitialized(bool available) override {
        (new SerializableThreadGarbageCollectorInitializedEvent(available))->Queue();
    }

    void OnThreadGarbageCollectorInitializationFailed() override {
        (new SerializableThreadGarbageCollectorInitializationFailedEvent())->Queue();
    }

    void OnThreadGarbageCollectionRequested(
        Threads::ThreadGarbageCollectionExecutionMode mode
    ) override {
        (new SerializableThreadGarbageCollectionRequestedEvent(mode))->Queue();
    }

    void OnThreadGarbageCollectionQueued(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new SerializableThreadGarbageCollectionQueuedEvent(result))->Queue();
    }

    void OnThreadGarbageCollectionRequestCoalesced(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new SerializableThreadGarbageCollectionRequestCoalescedEvent(result))->Queue();
    }

    void OnThreadGarbageCollectionStarted(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new SerializableThreadGarbageCollectionStartedEvent(result))->Queue();
    }

    void OnThreadGarbageCollectionCompleted(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new SerializableThreadGarbageCollectionCompletedEvent(result))->Queue();
    }

    void OnThreadGarbageCollectionFailed(
        const Threads::ThreadGarbageCollectionResult& result,
        std::exception_ptr cause
    ) override {
        (new SerializableThreadGarbageCollectionFailedEvent(
            result,
            Internal::DescribeThreadBridgeException(cause)
        ))->Queue();
    }

    void OnThreadGarbageCollectionFallbackStarted(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new SerializableThreadGarbageCollectionFallbackStartedEvent(result))->Queue();
    }
};

}
}
