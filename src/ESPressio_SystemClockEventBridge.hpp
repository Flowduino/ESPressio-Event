#pragma once

#include <ESPressio_ISystemClockObserver.hpp>
#include <ESPressio_SystemClock.hpp>
#include <ESPressio_TimingEvents.hpp>

namespace ESPressio {
namespace Event {

class SystemClockEventBridge final :
    public Timing::ISystemClockObserver<Timing::ClockTick> {
private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    SystemClockEventBridge() = default;

public:
    SystemClockEventBridge(const SystemClockEventBridge&) = delete;
    SystemClockEventBridge& operator=(const SystemClockEventBridge&) = delete;

    static SystemClockEventBridge& GetInstance() {
        static SystemClockEventBridge instance;
        return instance;
    }

    bool Initialize() {
        if (_initialized) return true;
        _observerHandle = Timing::SystemClock<>::GetInstance().RegisterObserver(this);
        _initialized = static_cast<bool>(_observerHandle);
        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const { return _initialized; }

    void OnSystemClockTimeSet(Timing::ClockTick before, Timing::ClockTick after, int64_t diff) override {
        (new SystemClockTimeChangedEvent(before, after, diff))->Queue();
    }

    void OnSystemClockSynchronizationSampleAccepted(Timing::ClockTick before, Timing::ClockTick after,
        int64_t diff, const Timing::ClockSynchronizationResult<Timing::ClockTick>& result,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) override {
        (new SynchronizationSampleAcceptedEvent(before, after, diff, result, status))->Queue();
    }

    void OnSystemClockSynchronized(Timing::ClockTick before, Timing::ClockTick after, int64_t diff,
        const Timing::ClockSynchronizationResult<Timing::ClockTick>& result,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) override {
        (new SystemClockSynchronizedEvent(before, after, diff, result, status))->Queue();
    }

    void OnSystemClockSynchronizationSampleRejected(
        const Timing::ClockSynchronizationResult<Timing::ClockTick>& result,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) override {
        (new SynchronizationSampleRejectedEvent(result, status))->Queue();
    }

    void OnSystemClockSynchronizationStateChanged(Timing::ClockSynchronizationState before,
        Timing::ClockSynchronizationState after,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) override {
        (new SynchronizationStateChangedEvent(before, after, status))->Queue();
    }

    void OnSystemClockSynchronizationReset(
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& before,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& after) override {
        (new SynchronizationResetEvent(before, after))->Queue();
    }

    void OnSystemClockSynchronizationConfigurationChanged(
        const Timing::ClockSynchronizationConfig& before,
        const Timing::ClockSynchronizationConfig& after) override {
        (new SynchronizationConfigurationChangedEvent(before, after))->Queue();
    }

    void OnSystemClockCallbackScheduled(Timing::ClockTick scheduled) override {
        (new SystemClockCallbackScheduledEvent(scheduled))->Queue();
    }

    void OnSystemClockCallbackScheduleFailed(Timing::ClockTick scheduled) override {
        (new SystemClockCallbackScheduleFailedEvent(scheduled))->Queue();
    }

    void OnSystemClockCallbackExecuted(Timing::ClockTick scheduled, Timing::ClockTick actual,
        int64_t diff) override {
        (new SystemClockCallbackExecutedEvent(scheduled, actual, diff))->Queue();
    }

    void OnSystemClockCallbackExecutionFailed(Timing::ClockTick scheduled, Timing::ClockTick actual,
        int64_t diff, std::exception_ptr cause) override {
        (new SystemClockCallbackExecutionFailedEvent(scheduled, actual, diff, cause))->Queue();
    }

    void OnSystemClockCallbacksCleared(std::size_t count) override {
        (new SystemClockCallbacksClearedEvent(count))->Queue();
    }
};

} // namespace Event
} // namespace ESPressio
