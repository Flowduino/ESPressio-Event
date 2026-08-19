#pragma once

#if !__has_include(<ESPressio_Serializable.hpp>)
#error "Serializable Thread Events require ESPressio-Serializable."
#endif

#include <cstdint>
#include <string>

#include <ESPressio_ThreadManagerTypes.hpp>
#include <ESPressio_ThreadGarbageCollectorTypes.hpp>

#include "../ESPressio_Event_Serializable.hpp"

namespace ESPressio {
namespace Event {

struct SerializableThreadSnapshotData {
    uint8_t ThreadID = 0;
    int32_t CoreID = 0;
    uint8_t State = 0;
    bool FreeOnTerminate = false;
    bool StartOnInitialize = true;

    static SerializableThreadSnapshotData From(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) {
        SerializableThreadSnapshotData value;
        value.ThreadID = snapshot.ThreadID;
        value.CoreID = snapshot.CoreID;
        value.State = static_cast<uint8_t>(snapshot.State);
        value.FreeOnTerminate = snapshot.FreeOnTerminate;
        value.StartOnInitialize = snapshot.StartOnInitialize;
        return value;
    }
};

#define ESPRESSIO_THREAD_SNAPSHOT_MEMBERS \
    uint8_t ThreadID = 0; \
    int32_t CoreID = 0; \
    uint8_t State = 0; \
    bool FreeOnTerminate = false; \
    bool StartOnInitialize = true;

#define ESPRESSIO_THREAD_SNAPSHOT_ASSIGN(s) \
    ThreadID = (s).ThreadID; \
    CoreID = (s).CoreID; \
    State = (s).State; \
    FreeOnTerminate = (s).FreeOnTerminate; \
    StartOnInitialize = (s).StartOnInitialize;

#define ESPRESSIO_THREAD_SNAPSHOT_PROPERTIES \
    ESPRESSIO_PROPERTY("threadID", ThreadID), \
    ESPRESSIO_PROPERTY("coreID", CoreID), \
    ESPRESSIO_PROPERTY("state", State), \
    ESPRESSIO_PROPERTY("freeOnTerminate", FreeOnTerminate), \
    ESPRESSIO_PROPERTY("startOnInitialize", StartOnInitialize)

#define ESPRESSIO_THREAD_CLEANUP_MEMBERS \
    uint32_t ThreadsExamined = 0; \
    uint32_t ThreadsClaimed = 0; \
    uint32_t ThreadsRemoved = 0; \
    uint32_t ThreadsDeleted = 0; \
    bool WasDeferred = false; \
    uint32_t ActiveIterationCount = 0; \
    uint32_t ThreadCountBefore = 0; \
    uint32_t ThreadCountAfter = 0;

#define ESPRESSIO_THREAD_CLEANUP_ASSIGN(r) \
    ThreadsExamined = static_cast<uint32_t>((r).ThreadsExamined); \
    ThreadsClaimed = static_cast<uint32_t>((r).ThreadsClaimed); \
    ThreadsRemoved = static_cast<uint32_t>((r).ThreadsRemoved); \
    ThreadsDeleted = static_cast<uint32_t>((r).ThreadsDeleted); \
    WasDeferred = (r).WasDeferred; \
    ActiveIterationCount = static_cast<uint32_t>((r).ActiveIterationCount); \
    ThreadCountBefore = static_cast<uint32_t>((r).ThreadCountBefore); \
    ThreadCountAfter = static_cast<uint32_t>((r).ThreadCountAfter);

#define ESPRESSIO_THREAD_CLEANUP_PROPERTIES \
    ESPRESSIO_PROPERTY("threadsExamined", ThreadsExamined), \
    ESPRESSIO_PROPERTY("threadsClaimed", ThreadsClaimed), \
    ESPRESSIO_PROPERTY("threadsRemoved", ThreadsRemoved), \
    ESPRESSIO_PROPERTY("threadsDeleted", ThreadsDeleted), \
    ESPRESSIO_PROPERTY("wasDeferred", WasDeferred), \
    ESPRESSIO_PROPERTY("activeIterationCount", ActiveIterationCount), \
    ESPRESSIO_PROPERTY("threadCountBefore", ThreadCountBefore), \
    ESPRESSIO_PROPERTY("threadCountAfter", ThreadCountAfter)

#define ESPRESSIO_GC_MEMBERS \
    uint8_t ExecutionMode = 0; \
    bool InfrastructureAvailable = false; \
    bool RequestQueued = false; \
    bool Completed = false; \
    bool Failed = false; \
    ESPRESSIO_THREAD_CLEANUP_MEMBERS

#define ESPRESSIO_GC_ASSIGN(r) \
    ExecutionMode = static_cast<uint8_t>((r).ExecutionMode); \
    InfrastructureAvailable = (r).InfrastructureAvailable; \
    RequestQueued = (r).RequestQueued; \
    Completed = (r).Completed; \
    Failed = (r).Failed; \
    ESPRESSIO_THREAD_CLEANUP_ASSIGN((r).ManagerResult)

#define ESPRESSIO_GC_PROPERTIES \
    ESPRESSIO_PROPERTY("executionMode", ExecutionMode), \
    ESPRESSIO_PROPERTY("infrastructureAvailable", InfrastructureAvailable), \
    ESPRESSIO_PROPERTY("requestQueued", RequestQueued), \
    ESPRESSIO_PROPERTY("completed", Completed), \
    ESPRESSIO_PROPERTY("failed", Failed), \
    ESPRESSIO_THREAD_CLEANUP_PROPERTIES

#define ESPRESSIO_DEFINE_SERIALIZABLE_SNAPSHOT_EVENT(CLASS_NAME) \
class CLASS_NAME final : public SerializableEvent<CLASS_NAME> { \
public: \
    ESPRESSIO_THREAD_SNAPSHOT_MEMBERS \
    CLASS_NAME() = default; \
    explicit CLASS_NAME(const Threads::ThreadManagerThreadSnapshot& snapshot) { \
        auto data = SerializableThreadSnapshotData::From(snapshot); \
        ESPRESSIO_THREAD_SNAPSHOT_ASSIGN(data) \
    } \
    ESPRESSIO_SERIALIZABLE_TYPE(CLASS_NAME) \
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1) \
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_THREAD_SNAPSHOT_PROPERTIES) \
};

ESPRESSIO_DEFINE_SERIALIZABLE_SNAPSHOT_EVENT(SerializableThreadRegisteredEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_SNAPSHOT_EVENT(SerializableThreadRemovedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_SNAPSHOT_EVENT(SerializableThreadCleanupClaimedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_SNAPSHOT_EVENT(SerializableThreadTerminationDispatchQueuedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_SNAPSHOT_EVENT(SerializableThreadTerminationDispatchQueueFailedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_SNAPSHOT_EVENT(SerializableThreadTerminationDispatchStartedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_SNAPSHOT_EVENT(SerializableThreadTerminationDispatchCompletedEvent)

#undef ESPRESSIO_DEFINE_SERIALIZABLE_SNAPSHOT_EVENT

class SerializableThreadRegistrationFailedEvent final :
    public SerializableEvent<SerializableThreadRegistrationFailedEvent> {
public:
    uint64_t ThreadAddress = 0;
    std::string ExceptionMessage;

    SerializableThreadRegistrationFailedEvent() = default;

    SerializableThreadRegistrationFailedEvent(
        uint64_t threadAddress,
        const std::string& message
    ) :
        ThreadAddress(threadAddress),
        ExceptionMessage(message) {
    }

    ESPRESSIO_SERIALIZABLE_TYPE(SerializableThreadRegistrationFailedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("threadAddress", ThreadAddress),
        ESPRESSIO_PROPERTY("exceptionMessage", ExceptionMessage)
    )
};

#define ESPRESSIO_DEFINE_SERIALIZABLE_CLEANUP_EVENT(CLASS_NAME) \
class CLASS_NAME final : public SerializableEvent<CLASS_NAME> { \
public: \
    ESPRESSIO_THREAD_CLEANUP_MEMBERS \
    CLASS_NAME() = default; \
    explicit CLASS_NAME(const Threads::ThreadManagerCleanupResult& result) { \
        ESPRESSIO_THREAD_CLEANUP_ASSIGN(result) \
    } \
    ESPRESSIO_SERIALIZABLE_TYPE(CLASS_NAME) \
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1) \
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_THREAD_CLEANUP_PROPERTIES) \
};

ESPRESSIO_DEFINE_SERIALIZABLE_CLEANUP_EVENT(SerializableThreadCleanupDeferredEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_CLEANUP_EVENT(SerializableThreadCleanupStartedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_CLEANUP_EVENT(SerializableThreadCleanupCompletedEvent)

#undef ESPRESSIO_DEFINE_SERIALIZABLE_CLEANUP_EVENT

class SerializableThreadCleanupFailedEvent final :
    public SerializableEvent<SerializableThreadCleanupFailedEvent> {
public:
    ESPRESSIO_THREAD_CLEANUP_MEMBERS
    std::string ExceptionMessage;

    SerializableThreadCleanupFailedEvent() = default;

    SerializableThreadCleanupFailedEvent(
        const Threads::ThreadManagerCleanupResult& result,
        const std::string& message
    ) :
        ExceptionMessage(message) {
        ESPRESSIO_THREAD_CLEANUP_ASSIGN(result)
    }

    ESPRESSIO_SERIALIZABLE_TYPE(SerializableThreadCleanupFailedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_THREAD_CLEANUP_PROPERTIES,
        ESPRESSIO_PROPERTY("exceptionMessage", ExceptionMessage)
    )
};

class SerializableThreadManagerInitializationCompletedEvent final :
    public SerializableEvent<SerializableThreadManagerInitializationCompletedEvent> {
public:
    uint32_t ThreadsExamined = 0;
    uint32_t ThreadsInitializedSuccessfully = 0;
    uint32_t ThreadsInitializationFailed = 0;

    SerializableThreadManagerInitializationCompletedEvent() = default;

    explicit SerializableThreadManagerInitializationCompletedEvent(
        const Threads::ThreadManagerInitializationResult& result
    ) :
        ThreadsExamined(static_cast<uint32_t>(result.ThreadsExamined)),
        ThreadsInitializedSuccessfully(
            static_cast<uint32_t>(result.ThreadsInitializedSuccessfully)
        ),
        ThreadsInitializationFailed(
            static_cast<uint32_t>(result.ThreadsInitializationFailed)
        ) {
    }

    ESPRESSIO_SERIALIZABLE_TYPE(SerializableThreadManagerInitializationCompletedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("threadsExamined", ThreadsExamined),
        ESPRESSIO_PROPERTY("threadsInitializedSuccessfully", ThreadsInitializedSuccessfully),
        ESPRESSIO_PROPERTY("threadsInitializationFailed", ThreadsInitializationFailed)
    )
};

class SerializableThreadGarbageCollectorInitializedEvent final :
    public SerializableEvent<SerializableThreadGarbageCollectorInitializedEvent> {
public:
    bool Available = false;

    SerializableThreadGarbageCollectorInitializedEvent() = default;
    explicit SerializableThreadGarbageCollectorInitializedEvent(bool available)
        : Available(available) {}

    ESPRESSIO_SERIALIZABLE_TYPE(SerializableThreadGarbageCollectorInitializedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("available", Available)
    )
};

class SerializableThreadGarbageCollectorInitializationFailedEvent final :
    public SerializableEvent<SerializableThreadGarbageCollectorInitializationFailedEvent> {
public:
    ESPRESSIO_SERIALIZABLE_TYPE(SerializableThreadGarbageCollectorInitializationFailedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES()
};

class SerializableThreadGarbageCollectionRequestedEvent final :
    public SerializableEvent<SerializableThreadGarbageCollectionRequestedEvent> {
public:
    uint8_t ExecutionMode = 0;

    SerializableThreadGarbageCollectionRequestedEvent() = default;
    explicit SerializableThreadGarbageCollectionRequestedEvent(
        Threads::ThreadGarbageCollectionExecutionMode mode
    ) :
        ExecutionMode(static_cast<uint8_t>(mode)) {
    }

    ESPRESSIO_SERIALIZABLE_TYPE(SerializableThreadGarbageCollectionRequestedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("executionMode", ExecutionMode)
    )
};

#define ESPRESSIO_DEFINE_SERIALIZABLE_GC_EVENT(CLASS_NAME) \
class CLASS_NAME final : public SerializableEvent<CLASS_NAME> { \
public: \
    ESPRESSIO_GC_MEMBERS \
    CLASS_NAME() = default; \
    explicit CLASS_NAME(const Threads::ThreadGarbageCollectionResult& result) { \
        ESPRESSIO_GC_ASSIGN(result) \
    } \
    ESPRESSIO_SERIALIZABLE_TYPE(CLASS_NAME) \
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1) \
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_GC_PROPERTIES) \
};

ESPRESSIO_DEFINE_SERIALIZABLE_GC_EVENT(SerializableThreadGarbageCollectionQueuedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_GC_EVENT(SerializableThreadGarbageCollectionRequestCoalescedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_GC_EVENT(SerializableThreadGarbageCollectionStartedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_GC_EVENT(SerializableThreadGarbageCollectionCompletedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_GC_EVENT(SerializableThreadGarbageCollectionFallbackStartedEvent)

#undef ESPRESSIO_DEFINE_SERIALIZABLE_GC_EVENT

class SerializableThreadGarbageCollectionFailedEvent final :
    public SerializableEvent<SerializableThreadGarbageCollectionFailedEvent> {
public:
    ESPRESSIO_GC_MEMBERS
    std::string ExceptionMessage;

    SerializableThreadGarbageCollectionFailedEvent() = default;

    SerializableThreadGarbageCollectionFailedEvent(
        const Threads::ThreadGarbageCollectionResult& result,
        const std::string& message
    ) :
        ExceptionMessage(message) {
        ESPRESSIO_GC_ASSIGN(result)
    }

    ESPRESSIO_SERIALIZABLE_TYPE(SerializableThreadGarbageCollectionFailedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_GC_PROPERTIES,
        ESPRESSIO_PROPERTY("exceptionMessage", ExceptionMessage)
    )
};

class SerializableThreadTerminationDispatcherInitializedEvent final :
    public SerializableEvent<SerializableThreadTerminationDispatcherInitializedEvent> {
public:
    bool Available = false;

    SerializableThreadTerminationDispatcherInitializedEvent() = default;
    explicit SerializableThreadTerminationDispatcherInitializedEvent(bool available)
        : Available(available) {}

    ESPRESSIO_SERIALIZABLE_TYPE(SerializableThreadTerminationDispatcherInitializedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("available", Available)
    )
};

#undef ESPRESSIO_GC_PROPERTIES
#undef ESPRESSIO_GC_ASSIGN
#undef ESPRESSIO_GC_MEMBERS
#undef ESPRESSIO_THREAD_CLEANUP_PROPERTIES
#undef ESPRESSIO_THREAD_CLEANUP_ASSIGN
#undef ESPRESSIO_THREAD_CLEANUP_MEMBERS
#undef ESPRESSIO_THREAD_SNAPSHOT_PROPERTIES
#undef ESPRESSIO_THREAD_SNAPSHOT_ASSIGN
#undef ESPRESSIO_THREAD_SNAPSHOT_MEMBERS

} // namespace Event
} // namespace ESPressio
