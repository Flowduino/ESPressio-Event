#pragma once

#include <exception>
#include <string>

#include <ESPressio_ISystemClockObserver.hpp>
#include <ESPressio_SystemClock.hpp>
#include <ESPressio_TimingEvents_Serializable.hpp>

namespace ESPressio {
namespace Event {

class SerializableSystemClockEventBridge final :
    public Timing::ISystemClockObserver<Timing::ClockTick> {
private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    SerializableSystemClockEventBridge() = default;

    static std::string DescribeException(std::exception_ptr cause) {
        if (!cause) return std::string();
        try {
            std::rethrow_exception(cause);
        } catch (const std::exception& exception) {
            return exception.what();
        } catch (...) {
            return "Unknown exception";
        }
    }

public:
    SerializableSystemClockEventBridge(const SerializableSystemClockEventBridge&) = delete;
    SerializableSystemClockEventBridge& operator=(const SerializableSystemClockEventBridge&) = delete;

    static SerializableSystemClockEventBridge& GetInstance() {
        static SerializableSystemClockEventBridge instance;
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
        (new SerializableSystemClockTimeChangedEvent(before, after, diff))->Queue();
    }
    void OnSystemClockSynchronizationSampleAccepted(Timing::ClockTick before, Timing::ClockTick after,
        int64_t diff, const Timing::ClockSynchronizationResult<Timing::ClockTick>& result,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) override {
        (new SerializableSynchronizationSampleAcceptedEvent(before, after, diff, result, status))->Queue();
    }
    void OnSystemClockSynchronized(Timing::ClockTick before, Timing::ClockTick after, int64_t diff,
        const Timing::ClockSynchronizationResult<Timing::ClockTick>& result,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) override {
        (new SerializableSystemClockSynchronizedEvent(before, after, diff, result, status))->Queue();
    }
    void OnSystemClockSynchronizationSampleRejected(
        const Timing::ClockSynchronizationResult<Timing::ClockTick>& result,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) override {
        (new SerializableSynchronizationSampleRejectedEvent(result, status))->Queue();
    }
    void OnSystemClockSynchronizationStateChanged(Timing::ClockSynchronizationState before,
        Timing::ClockSynchronizationState after,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& status) override {
        (new SerializableSynchronizationStateChangedEvent(before, after, status))->Queue();
    }
    void OnSystemClockSynchronizationReset(
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& before,
        const Timing::ClockSynchronizationStatus<Timing::ClockTick>& after) override {
        (new SerializableSynchronizationResetEvent(before, after))->Queue();
    }
    void OnSystemClockSynchronizationConfigurationChanged(
        const Timing::ClockSynchronizationConfig& before,
        const Timing::ClockSynchronizationConfig& after) override {
        (new SerializableSynchronizationConfigurationChangedEvent(before, after))->Queue();
    }
    void OnSystemClockCallbackScheduled(Timing::ClockTick scheduled) override {
        (new SerializableSystemClockCallbackScheduledEvent(scheduled))->Queue();
    }
    void OnSystemClockCallbackScheduleFailed(Timing::ClockTick scheduled) override {
        (new SerializableSystemClockCallbackScheduleFailedEvent(scheduled))->Queue();
    }
    void OnSystemClockCallbackExecuted(Timing::ClockTick scheduled, Timing::ClockTick actual,
        int64_t diff) override {
        (new SerializableSystemClockCallbackExecutedEvent(scheduled, actual, diff))->Queue();
    }
    void OnSystemClockCallbackExecutionFailed(Timing::ClockTick scheduled, Timing::ClockTick actual,
        int64_t diff, std::exception_ptr cause) override {
        (new SerializableSystemClockCallbackExecutionFailedEvent(
            scheduled, actual, diff, DescribeException(cause)))->Queue();
    }
    void OnSystemClockCallbacksCleared(std::size_t count) override {
        (new SerializableSystemClockCallbacksClearedEvent(count))->Queue();
    }
};

} // namespace Event
} // namespace ESPressio
