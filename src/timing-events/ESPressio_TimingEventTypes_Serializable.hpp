#pragma once

#if !__has_include(<ESPressio_Serializable.hpp>)
#error "Serializable Timing Events require ESPressio-Serializable."
#endif

#include <cstdint>
#include <string>

#include <ESPressio_Event_Serializable.hpp>
#include <ESPressio_ClockSynchronization.hpp>

namespace ESPressio {
namespace Event {

struct SerializableSynchronizationSnapshot {
    bool Accepted = false;
    int64_t MeasuredOffsetNanoseconds = 0;
    int64_t FilteredOffsetNanoseconds = 0;
    uint64_t RoundTripDelayNanoseconds = 0;
    int64_t PendingPhaseCorrectionNanoseconds = 0;
    int64_t AppliedCorrectionNanoseconds = 0;
    double EstimatedDriftPpm = 0.0;
    uint32_t AcceptedSampleCount = 0;
    uint32_t RejectedSampleCount = 0;
    uint8_t SynchronizationState = 0;
    uint64_t LastAcceptedSampleLocalTime = 0;
    bool HasAcceptedSample = false;

    static SerializableSynchronizationSnapshot From(
        const Timing::ClockSynchronizationResult<Timing::ClockTick>& result,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status
    ) {
        SerializableSynchronizationSnapshot value;
        value.Accepted = result.Accepted;
        value.MeasuredOffsetNanoseconds = result.MeasuredOffsetNanoseconds;
        value.FilteredOffsetNanoseconds = status.FilteredOffsetNanoseconds;
        value.RoundTripDelayNanoseconds = result.RoundTripDelayNanoseconds;
        value.PendingPhaseCorrectionNanoseconds = status.PendingPhaseCorrectionNanoseconds;
        value.AppliedCorrectionNanoseconds = status.AppliedCorrectionNanoseconds;
        value.EstimatedDriftPpm = status.EstimatedDriftPpm;
        value.AcceptedSampleCount = status.AcceptedSampleCount;
        value.RejectedSampleCount = status.RejectedSampleCount;
        value.SynchronizationState = static_cast<uint8_t>(status.State);
        value.LastAcceptedSampleLocalTime = status.LastAcceptedSampleLocalTime;
        value.HasAcceptedSample = status.HasAcceptedSample;
        return value;
    }
};

#define ESPRESSIO_TIMING_SNAPSHOT_MEMBERS \
    bool Accepted = false; \
    int64_t MeasuredOffsetNanoseconds = 0; \
    int64_t FilteredOffsetNanoseconds = 0; \
    uint64_t RoundTripDelayNanoseconds = 0; \
    int64_t PendingPhaseCorrectionNanoseconds = 0; \
    int64_t AppliedCorrectionNanoseconds = 0; \
    double EstimatedDriftPpm = 0.0; \
    uint32_t AcceptedSampleCount = 0; \
    uint32_t RejectedSampleCount = 0; \
    uint8_t SynchronizationState = 0; \
    uint64_t LastAcceptedSampleLocalTime = 0; \
    bool HasAcceptedSample = false;

#define ESPRESSIO_TIMING_SNAPSHOT_ASSIGN(snapshot) \
    Accepted = (snapshot).Accepted; \
    MeasuredOffsetNanoseconds = (snapshot).MeasuredOffsetNanoseconds; \
    FilteredOffsetNanoseconds = (snapshot).FilteredOffsetNanoseconds; \
    RoundTripDelayNanoseconds = (snapshot).RoundTripDelayNanoseconds; \
    PendingPhaseCorrectionNanoseconds = (snapshot).PendingPhaseCorrectionNanoseconds; \
    AppliedCorrectionNanoseconds = (snapshot).AppliedCorrectionNanoseconds; \
    EstimatedDriftPpm = (snapshot).EstimatedDriftPpm; \
    AcceptedSampleCount = (snapshot).AcceptedSampleCount; \
    RejectedSampleCount = (snapshot).RejectedSampleCount; \
    SynchronizationState = (snapshot).SynchronizationState; \
    LastAcceptedSampleLocalTime = (snapshot).LastAcceptedSampleLocalTime; \
    HasAcceptedSample = (snapshot).HasAcceptedSample;

#define ESPRESSIO_TIMING_SNAPSHOT_PROPERTIES \
    ESPRESSIO_PROPERTY("accepted", Accepted), \
    ESPRESSIO_PROPERTY("measuredOffsetNanoseconds", MeasuredOffsetNanoseconds), \
    ESPRESSIO_PROPERTY("filteredOffsetNanoseconds", FilteredOffsetNanoseconds), \
    ESPRESSIO_PROPERTY("roundTripDelayNanoseconds", RoundTripDelayNanoseconds), \
    ESPRESSIO_PROPERTY("pendingPhaseCorrectionNanoseconds", PendingPhaseCorrectionNanoseconds), \
    ESPRESSIO_PROPERTY("appliedCorrectionNanoseconds", AppliedCorrectionNanoseconds), \
    ESPRESSIO_PROPERTY("estimatedDriftPpm", EstimatedDriftPpm), \
    ESPRESSIO_PROPERTY("acceptedSampleCount", AcceptedSampleCount), \
    ESPRESSIO_PROPERTY("rejectedSampleCount", RejectedSampleCount), \
    ESPRESSIO_PROPERTY("synchronizationState", SynchronizationState), \
    ESPRESSIO_PROPERTY("lastAcceptedSampleLocalTime", LastAcceptedSampleLocalTime), \
    ESPRESSIO_PROPERTY("hasAcceptedSample", HasAcceptedSample)

class SerializableSystemClockTimeChangedEvent final :
    public SerializableEvent<SerializableSystemClockTimeChangedEvent> {
public:
    uint64_t PreviousTimeNanoseconds = 0;
    uint64_t NewTimeNanoseconds = 0;
    int64_t DifferenceNanoseconds = 0;
    SerializableSystemClockTimeChangedEvent() = default;
    SerializableSystemClockTimeChangedEvent(uint64_t before, uint64_t after, int64_t diff)
        : PreviousTimeNanoseconds(before), NewTimeNanoseconds(after), DifferenceNanoseconds(diff) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SerializableSystemClockTimeChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("previousTimeNanoseconds", PreviousTimeNanoseconds),
        ESPRESSIO_PROPERTY("newTimeNanoseconds", NewTimeNanoseconds),
        ESPRESSIO_PROPERTY("differenceNanoseconds", DifferenceNanoseconds)
    )
};

#define ESPRESSIO_DEFINE_SERIALIZABLE_SYNC_EVENT(CLASS_NAME) \
class CLASS_NAME final : public SerializableEvent<CLASS_NAME> { \
public: \
    uint64_t ClockBeforeNanoseconds = 0; \
    uint64_t ClockAfterNanoseconds = 0; \
    int64_t ImmediateDifferenceNanoseconds = 0; \
    ESPRESSIO_TIMING_SNAPSHOT_MEMBERS \
    CLASS_NAME() = default; \
    CLASS_NAME(uint64_t before, uint64_t after, int64_t diff, \
        const Timing::ClockSynchronizationResult<Timing::ClockTick>& result, \
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) \
        : ClockBeforeNanoseconds(before), ClockAfterNanoseconds(after), ImmediateDifferenceNanoseconds(diff) { \
        auto snapshot = SerializableSynchronizationSnapshot::From(result, status); \
        ESPRESSIO_TIMING_SNAPSHOT_ASSIGN(snapshot) \
    } \
    ESPRESSIO_SERIALIZABLE_TYPE(CLASS_NAME) \
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1) \
    ESPRESSIO_SERIALIZABLE_PROPERTIES( \
        ESPRESSIO_PROPERTY("clockBeforeNanoseconds", ClockBeforeNanoseconds), \
        ESPRESSIO_PROPERTY("clockAfterNanoseconds", ClockAfterNanoseconds), \
        ESPRESSIO_PROPERTY("immediateDifferenceNanoseconds", ImmediateDifferenceNanoseconds), \
        ESPRESSIO_TIMING_SNAPSHOT_PROPERTIES \
    ) \
};

ESPRESSIO_DEFINE_SERIALIZABLE_SYNC_EVENT(SerializableSynchronizationSampleAcceptedEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_SYNC_EVENT(SerializableSystemClockSynchronizedEvent)

class SerializableSynchronizationSampleRejectedEvent final :
    public SerializableEvent<SerializableSynchronizationSampleRejectedEvent> {
public:
    ESPRESSIO_TIMING_SNAPSHOT_MEMBERS
    SerializableSynchronizationSampleRejectedEvent() = default;
    SerializableSynchronizationSampleRejectedEvent(
        const Timing::ClockSynchronizationResult<Timing::ClockTick>& result,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) {
        auto snapshot = SerializableSynchronizationSnapshot::From(result, status);
        ESPRESSIO_TIMING_SNAPSHOT_ASSIGN(snapshot)
    }
    ESPRESSIO_SERIALIZABLE_TYPE(SerializableSynchronizationSampleRejectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_TIMING_SNAPSHOT_PROPERTIES)
};

class SerializableSynchronizationStateChangedEvent final :
    public SerializableEvent<SerializableSynchronizationStateChangedEvent> {
public:
    uint8_t PreviousState = 0;
    uint8_t NewState = 0;
    ESPRESSIO_TIMING_SNAPSHOT_MEMBERS
    SerializableSynchronizationStateChangedEvent() = default;
    SerializableSynchronizationStateChangedEvent(Timing::ClockSynchronizationState before,
        Timing::ClockSynchronizationState after,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status)
        : PreviousState(static_cast<uint8_t>(before)), NewState(static_cast<uint8_t>(after)) {
        Timing::ClockSynchronizationResult<Timing::ClockTick> result;
        result.Accepted = status.HasAcceptedSample;
        result.MeasuredOffsetNanoseconds = status.LastMeasuredOffsetNanoseconds;
        result.FilteredOffsetNanoseconds = status.FilteredOffsetNanoseconds;
        result.RoundTripDelayNanoseconds = status.LastRoundTripDelayNanoseconds;
        result.EstimatedDriftPpm = status.EstimatedDriftPpm;
        result.AcceptedSampleCount = status.AcceptedSampleCount;
        result.RejectedSampleCount = status.RejectedSampleCount;
        auto snapshot = SerializableSynchronizationSnapshot::From(result, status);
        ESPRESSIO_TIMING_SNAPSHOT_ASSIGN(snapshot)
    }
    ESPRESSIO_SERIALIZABLE_TYPE(SerializableSynchronizationStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("previousState", PreviousState),
        ESPRESSIO_PROPERTY("newState", NewState),
        ESPRESSIO_TIMING_SNAPSHOT_PROPERTIES
    )
};

class SerializableSynchronizationResetEvent final :
    public SerializableEvent<SerializableSynchronizationResetEvent> {
public:
    uint8_t PreviousState = 0;
    uint8_t NewState = 0;
    int64_t PreviousFilteredOffsetNanoseconds = 0;
    int64_t NewFilteredOffsetNanoseconds = 0;
    uint32_t PreviousAcceptedSampleCount = 0;
    uint32_t NewAcceptedSampleCount = 0;
    SerializableSynchronizationResetEvent() = default;
    SerializableSynchronizationResetEvent(
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& before,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& after)
        : PreviousState(static_cast<uint8_t>(before.State)), NewState(static_cast<uint8_t>(after.State)),
          PreviousFilteredOffsetNanoseconds(before.FilteredOffsetNanoseconds),
          NewFilteredOffsetNanoseconds(after.FilteredOffsetNanoseconds),
          PreviousAcceptedSampleCount(before.AcceptedSampleCount),
          NewAcceptedSampleCount(after.AcceptedSampleCount) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SerializableSynchronizationResetEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("previousState", PreviousState),
        ESPRESSIO_PROPERTY("newState", NewState),
        ESPRESSIO_PROPERTY("previousFilteredOffsetNanoseconds", PreviousFilteredOffsetNanoseconds),
        ESPRESSIO_PROPERTY("newFilteredOffsetNanoseconds", NewFilteredOffsetNanoseconds),
        ESPRESSIO_PROPERTY("previousAcceptedSampleCount", PreviousAcceptedSampleCount),
        ESPRESSIO_PROPERTY("newAcceptedSampleCount", NewAcceptedSampleCount)
    )
};

class SerializableSynchronizationConfigurationChangedEvent final :
    public SerializableEvent<SerializableSynchronizationConfigurationChangedEvent> {
public:
    uint64_t PreviousMaximumRoundTripDelayNanoseconds = 0;
    uint64_t NewMaximumRoundTripDelayNanoseconds = 0;
    uint32_t PreviousMaximumSlewRatePpm = 0;
    uint32_t NewMaximumSlewRatePpm = 0;
    double PreviousMaximumDriftCorrectionPpm = 0.0;
    double NewMaximumDriftCorrectionPpm = 0.0;
    double PreviousOffsetFilterWeight = 0.0;
    double NewOffsetFilterWeight = 0.0;
    double PreviousDriftFilterWeight = 0.0;
    double NewDriftFilterWeight = 0.0;
    uint64_t PreviousDriftLearningPhaseThresholdNanoseconds = 0;
    uint64_t NewDriftLearningPhaseThresholdNanoseconds = 0;
    uint64_t PreviousMinimumDriftLearningIntervalNanoseconds = 0;
    uint64_t NewMinimumDriftLearningIntervalNanoseconds = 0;
    uint64_t PreviousSynchronizationToleranceNanoseconds = 0;
    uint64_t NewSynchronizationToleranceNanoseconds = 0;
    uint32_t PreviousMinimumSamplesForSynchronizedState = 0;
    uint32_t NewMinimumSamplesForSynchronizedState = 0;
    uint64_t PreviousMaximumSampleAgeNanoseconds = 0;
    uint64_t NewMaximumSampleAgeNanoseconds = 0;

    SerializableSynchronizationConfigurationChangedEvent() = default;
    SerializableSynchronizationConfigurationChangedEvent(
        const Timing::ClockSynchronizationConfig& before,
        const Timing::ClockSynchronizationConfig& after)
        : PreviousMaximumRoundTripDelayNanoseconds(before.MaximumRoundTripDelayNanoseconds),
          NewMaximumRoundTripDelayNanoseconds(after.MaximumRoundTripDelayNanoseconds),
          PreviousMaximumSlewRatePpm(before.MaximumSlewRatePpm),
          NewMaximumSlewRatePpm(after.MaximumSlewRatePpm),
          PreviousMaximumDriftCorrectionPpm(before.MaximumDriftCorrectionPpm),
          NewMaximumDriftCorrectionPpm(after.MaximumDriftCorrectionPpm),
          PreviousOffsetFilterWeight(before.OffsetFilterWeight),
          NewOffsetFilterWeight(after.OffsetFilterWeight),
          PreviousDriftFilterWeight(before.DriftFilterWeight),
          NewDriftFilterWeight(after.DriftFilterWeight),
          PreviousDriftLearningPhaseThresholdNanoseconds(before.DriftLearningPhaseThresholdNanoseconds),
          NewDriftLearningPhaseThresholdNanoseconds(after.DriftLearningPhaseThresholdNanoseconds),
          PreviousMinimumDriftLearningIntervalNanoseconds(before.MinimumDriftLearningIntervalNanoseconds),
          NewMinimumDriftLearningIntervalNanoseconds(after.MinimumDriftLearningIntervalNanoseconds),
          PreviousSynchronizationToleranceNanoseconds(before.SynchronizationToleranceNanoseconds),
          NewSynchronizationToleranceNanoseconds(after.SynchronizationToleranceNanoseconds),
          PreviousMinimumSamplesForSynchronizedState(before.MinimumSamplesForSynchronizedState),
          NewMinimumSamplesForSynchronizedState(after.MinimumSamplesForSynchronizedState),
          PreviousMaximumSampleAgeNanoseconds(before.MaximumSampleAgeNanoseconds),
          NewMaximumSampleAgeNanoseconds(after.MaximumSampleAgeNanoseconds) {}

    ESPRESSIO_SERIALIZABLE_TYPE(SerializableSynchronizationConfigurationChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("previousMaximumRoundTripDelayNanoseconds", PreviousMaximumRoundTripDelayNanoseconds),
        ESPRESSIO_PROPERTY("newMaximumRoundTripDelayNanoseconds", NewMaximumRoundTripDelayNanoseconds),
        ESPRESSIO_PROPERTY("previousMaximumSlewRatePpm", PreviousMaximumSlewRatePpm),
        ESPRESSIO_PROPERTY("newMaximumSlewRatePpm", NewMaximumSlewRatePpm),
        ESPRESSIO_PROPERTY("previousMaximumDriftCorrectionPpm", PreviousMaximumDriftCorrectionPpm),
        ESPRESSIO_PROPERTY("newMaximumDriftCorrectionPpm", NewMaximumDriftCorrectionPpm),
        ESPRESSIO_PROPERTY("previousOffsetFilterWeight", PreviousOffsetFilterWeight),
        ESPRESSIO_PROPERTY("newOffsetFilterWeight", NewOffsetFilterWeight),
        ESPRESSIO_PROPERTY("previousDriftFilterWeight", PreviousDriftFilterWeight),
        ESPRESSIO_PROPERTY("newDriftFilterWeight", NewDriftFilterWeight),
        ESPRESSIO_PROPERTY("previousDriftLearningPhaseThresholdNanoseconds", PreviousDriftLearningPhaseThresholdNanoseconds),
        ESPRESSIO_PROPERTY("newDriftLearningPhaseThresholdNanoseconds", NewDriftLearningPhaseThresholdNanoseconds),
        ESPRESSIO_PROPERTY("previousMinimumDriftLearningIntervalNanoseconds", PreviousMinimumDriftLearningIntervalNanoseconds),
        ESPRESSIO_PROPERTY("newMinimumDriftLearningIntervalNanoseconds", NewMinimumDriftLearningIntervalNanoseconds),
        ESPRESSIO_PROPERTY("previousSynchronizationToleranceNanoseconds", PreviousSynchronizationToleranceNanoseconds),
        ESPRESSIO_PROPERTY("newSynchronizationToleranceNanoseconds", NewSynchronizationToleranceNanoseconds),
        ESPRESSIO_PROPERTY("previousMinimumSamplesForSynchronizedState", PreviousMinimumSamplesForSynchronizedState),
        ESPRESSIO_PROPERTY("newMinimumSamplesForSynchronizedState", NewMinimumSamplesForSynchronizedState),
        ESPRESSIO_PROPERTY("previousMaximumSampleAgeNanoseconds", PreviousMaximumSampleAgeNanoseconds),
        ESPRESSIO_PROPERTY("newMaximumSampleAgeNanoseconds", NewMaximumSampleAgeNanoseconds)
    )
};

#define ESPRESSIO_DEFINE_SERIALIZABLE_SCHEDULE_EVENT(CLASS_NAME) \
class CLASS_NAME final : public SerializableEvent<CLASS_NAME> { \
public: uint64_t ScheduledTimeNanoseconds = 0; \
    CLASS_NAME() = default; explicit CLASS_NAME(uint64_t scheduled) : ScheduledTimeNanoseconds(scheduled) {} \
    ESPRESSIO_SERIALIZABLE_TYPE(CLASS_NAME) ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1) \
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("scheduledTimeNanoseconds", ScheduledTimeNanoseconds)) \
};

ESPRESSIO_DEFINE_SERIALIZABLE_SCHEDULE_EVENT(SerializableSystemClockCallbackScheduledEvent)
ESPRESSIO_DEFINE_SERIALIZABLE_SCHEDULE_EVENT(SerializableSystemClockCallbackScheduleFailedEvent)

class SerializableSystemClockCallbackExecutedEvent final :
    public SerializableEvent<SerializableSystemClockCallbackExecutedEvent> {
public:
    uint64_t ScheduledTimeNanoseconds = 0;
    uint64_t ActualTimeNanoseconds = 0;
    int64_t DifferenceNanoseconds = 0;
    SerializableSystemClockCallbackExecutedEvent() = default;
    SerializableSystemClockCallbackExecutedEvent(uint64_t scheduled, uint64_t actual, int64_t diff)
        : ScheduledTimeNanoseconds(scheduled), ActualTimeNanoseconds(actual), DifferenceNanoseconds(diff) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SerializableSystemClockCallbackExecutedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("scheduledTimeNanoseconds", ScheduledTimeNanoseconds),
        ESPRESSIO_PROPERTY("actualTimeNanoseconds", ActualTimeNanoseconds),
        ESPRESSIO_PROPERTY("differenceNanoseconds", DifferenceNanoseconds)
    )
};

class SerializableSystemClockCallbackExecutionFailedEvent final :
    public SerializableEvent<SerializableSystemClockCallbackExecutionFailedEvent> {
public:
    uint64_t ScheduledTimeNanoseconds = 0;
    uint64_t ActualTimeNanoseconds = 0;
    int64_t DifferenceNanoseconds = 0;
    std::string ExceptionMessage;
    SerializableSystemClockCallbackExecutionFailedEvent() = default;
    SerializableSystemClockCallbackExecutionFailedEvent(uint64_t scheduled, uint64_t actual, int64_t diff,
        const std::string& message)
        : ScheduledTimeNanoseconds(scheduled), ActualTimeNanoseconds(actual),
          DifferenceNanoseconds(diff), ExceptionMessage(message) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SerializableSystemClockCallbackExecutionFailedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("scheduledTimeNanoseconds", ScheduledTimeNanoseconds),
        ESPRESSIO_PROPERTY("actualTimeNanoseconds", ActualTimeNanoseconds),
        ESPRESSIO_PROPERTY("differenceNanoseconds", DifferenceNanoseconds),
        ESPRESSIO_PROPERTY("exceptionMessage", ExceptionMessage)
    )
};

class SerializableSystemClockCallbacksClearedEvent final :
    public SerializableEvent<SerializableSystemClockCallbacksClearedEvent> {
public:
    uint32_t ClearedCallbackCount = 0;
    SerializableSystemClockCallbacksClearedEvent() = default;
    explicit SerializableSystemClockCallbacksClearedEvent(std::size_t count)
        : ClearedCallbackCount(static_cast<uint32_t>(count)) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SerializableSystemClockCallbacksClearedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("clearedCallbackCount", ClearedCallbackCount)
    )
};

#undef ESPRESSIO_DEFINE_SERIALIZABLE_SYNC_EVENT
#undef ESPRESSIO_DEFINE_SERIALIZABLE_SCHEDULE_EVENT
#undef ESPRESSIO_TIMING_SNAPSHOT_PROPERTIES
#undef ESPRESSIO_TIMING_SNAPSHOT_ASSIGN
#undef ESPRESSIO_TIMING_SNAPSHOT_MEMBERS

} // namespace Event
} // namespace ESPressio
