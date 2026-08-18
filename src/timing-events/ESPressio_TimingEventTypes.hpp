#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>

#include <ESPressio_ClockSynchronization.hpp>
#include <ESPressio_Event.hpp>

namespace ESPressio {
namespace Event {

using TimingClockTick = Timing::ClockTick;
using SynchronizationResult = Timing::ClockSynchronizationResult<TimingClockTick>;
using SynchronizationStatus = Timing::ClockSynchronizationStatus<TimingClockTick>;

class SystemClockTimeChangedEvent final : public Event<> {
public:
    const TimingClockTick PreviousTimeNanoseconds;
    const TimingClockTick NewTimeNanoseconds;
    const int64_t DifferenceNanoseconds;
    SystemClockTimeChangedEvent(TimingClockTick previousTime, TimingClockTick newTime, int64_t difference)
        : PreviousTimeNanoseconds(previousTime), NewTimeNanoseconds(newTime), DifferenceNanoseconds(difference) {}
};

class SynchronizationSampleAcceptedEvent final : public Event<> {
public:
    const TimingClockTick ClockBeforeNanoseconds;
    const TimingClockTick ClockAfterNanoseconds;
    const int64_t ImmediateDifferenceNanoseconds;
    const SynchronizationResult Result;
    const SynchronizationStatus Status;
    SynchronizationSampleAcceptedEvent(TimingClockTick before, TimingClockTick after, int64_t difference,
        const SynchronizationResult& result, const SynchronizationStatus& status)
        : ClockBeforeNanoseconds(before), ClockAfterNanoseconds(after), ImmediateDifferenceNanoseconds(difference),
          Result(result), Status(status) {}
};

class SynchronizationSampleRejectedEvent final : public Event<> {
public:
    const SynchronizationResult Result;
    const SynchronizationStatus Status;
    SynchronizationSampleRejectedEvent(const SynchronizationResult& result, const SynchronizationStatus& status)
        : Result(result), Status(status) {}
};

class SystemClockSynchronizedEvent final : public Event<> {
public:
    const TimingClockTick ClockBeforeNanoseconds;
    const TimingClockTick ClockAfterNanoseconds;
    const int64_t ImmediateDifferenceNanoseconds;
    const SynchronizationResult Result;
    const SynchronizationStatus Status;
    SystemClockSynchronizedEvent(TimingClockTick before, TimingClockTick after, int64_t difference,
        const SynchronizationResult& result, const SynchronizationStatus& status)
        : ClockBeforeNanoseconds(before), ClockAfterNanoseconds(after), ImmediateDifferenceNanoseconds(difference),
          Result(result), Status(status) {}
};

class SynchronizationStateChangedEvent final : public Event<> {
public:
    const Timing::ClockSynchronizationState PreviousState;
    const Timing::ClockSynchronizationState NewState;
    const SynchronizationStatus Status;
    SynchronizationStateChangedEvent(Timing::ClockSynchronizationState previousState,
        Timing::ClockSynchronizationState newState, const SynchronizationStatus& status)
        : PreviousState(previousState), NewState(newState), Status(status) {}
};

class SynchronizationResetEvent final : public Event<> {
public:
    const SynchronizationStatus PreviousStatus;
    const SynchronizationStatus NewStatus;
    SynchronizationResetEvent(const SynchronizationStatus& previousStatus, const SynchronizationStatus& newStatus)
        : PreviousStatus(previousStatus), NewStatus(newStatus) {}
};

class SynchronizationConfigurationChangedEvent final : public Event<> {
public:
    const Timing::ClockSynchronizationConfig PreviousConfig;
    const Timing::ClockSynchronizationConfig NewConfig;
    SynchronizationConfigurationChangedEvent(const Timing::ClockSynchronizationConfig& previousConfig,
        const Timing::ClockSynchronizationConfig& newConfig)
        : PreviousConfig(previousConfig), NewConfig(newConfig) {}
};

class SystemClockCallbackScheduledEvent final : public Event<> {
public:
    const TimingClockTick ScheduledTimeNanoseconds;
    explicit SystemClockCallbackScheduledEvent(TimingClockTick scheduled) : ScheduledTimeNanoseconds(scheduled) {}
};

class SystemClockCallbackScheduleFailedEvent final : public Event<> {
public:
    const TimingClockTick ScheduledTimeNanoseconds;
    explicit SystemClockCallbackScheduleFailedEvent(TimingClockTick scheduled) : ScheduledTimeNanoseconds(scheduled) {}
};

class SystemClockCallbackExecutedEvent final : public Event<> {
public:
    const TimingClockTick ScheduledTimeNanoseconds;
    const TimingClockTick ActualTimeNanoseconds;
    const int64_t DifferenceNanoseconds;
    SystemClockCallbackExecutedEvent(TimingClockTick scheduled, TimingClockTick actual, int64_t difference)
        : ScheduledTimeNanoseconds(scheduled), ActualTimeNanoseconds(actual), DifferenceNanoseconds(difference) {}
};

class SystemClockCallbackExecutionFailedEvent final : public Event<> {
public:
    const TimingClockTick ScheduledTimeNanoseconds;
    const TimingClockTick ActualTimeNanoseconds;
    const int64_t DifferenceNanoseconds;
    const std::exception_ptr Cause;
    SystemClockCallbackExecutionFailedEvent(TimingClockTick scheduled, TimingClockTick actual,
        int64_t difference, std::exception_ptr cause)
        : ScheduledTimeNanoseconds(scheduled), ActualTimeNanoseconds(actual),
          DifferenceNanoseconds(difference), Cause(cause) {}
};

class SystemClockCallbacksClearedEvent final : public Event<> {
public:
    const std::size_t ClearedCallbackCount;
    explicit SystemClockCallbacksClearedEvent(std::size_t count) : ClearedCallbackCount(count) {}
};

} // namespace Event
} // namespace ESPressio
