#pragma once

#include <ESPressio_IThreadGarbageCollectorObserver.hpp>
#include <ESPressio_ThreadGarbageCollector.hpp>

#include "ESPressio_ThreadEvents.hpp"

namespace ESPressio {
namespace Event {

class ThreadGarbageCollectorEventBridge final :
    public Threads::IThreadGarbageCollectorObserver {

private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    ThreadGarbageCollectorEventBridge() = default;

public:
    ThreadGarbageCollectorEventBridge(const ThreadGarbageCollectorEventBridge&) = delete;
    ThreadGarbageCollectorEventBridge& operator=(const ThreadGarbageCollectorEventBridge&) = delete;

    static ThreadGarbageCollectorEventBridge& GetInstance() {
        static ThreadGarbageCollectorEventBridge instance;
        return instance;
    }

    bool Initialize() {
        if (_initialized) {
            return true;
        }

        _observerHandle =
            Threads::ThreadGarbageCollector::
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

    void OnThreadGarbageCollectorInitialized(
        bool available
    ) override {
        (new ThreadGarbageCollectorInitializedEvent(available))->Queue();
    }

    void OnThreadGarbageCollectorInitializationFailed() override {
        (new ThreadGarbageCollectorInitializationFailedEvent())->Queue();
    }

    void OnThreadGarbageCollectionRequested(
        Threads::ThreadGarbageCollectionExecutionMode mode
    ) override {
        (new ThreadGarbageCollectionRequestedEvent(mode))->Queue();
    }

    void OnThreadGarbageCollectionQueued(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new ThreadGarbageCollectionQueuedEvent(result))->Queue();
    }

    void OnThreadGarbageCollectionRequestCoalesced(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new ThreadGarbageCollectionRequestCoalescedEvent(result))->Queue();
    }

    void OnThreadGarbageCollectionStarted(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new ThreadGarbageCollectionStartedEvent(result))->Queue();
    }

    void OnThreadGarbageCollectionCompleted(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new ThreadGarbageCollectionCompletedEvent(result))->Queue();
    }

    void OnThreadGarbageCollectionFailed(
        const Threads::ThreadGarbageCollectionResult& result,
        std::exception_ptr cause
    ) override {
        (new ThreadGarbageCollectionFailedEvent(result, cause))->Queue();
    }

    void OnThreadGarbageCollectionFallbackStarted(
        const Threads::ThreadGarbageCollectionResult& result
    ) override {
        (new ThreadGarbageCollectionFallbackStartedEvent(result))->Queue();
    }
};

}
}
