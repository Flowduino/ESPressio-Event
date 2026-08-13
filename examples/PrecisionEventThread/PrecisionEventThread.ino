#include <ESPressio_Event.hpp>
#include <ESPressio_PrecisionEventThread.hpp>
#include <ESPressio_ThreadManager.hpp>

using namespace ESPressio;

class SetpointEvent final : public Event::Event {
    private:
        const int _setpoint;

    public:
        explicit SetpointEvent(int setpoint) : _setpoint(setpoint) { }
        int GetSetpoint() const { return _setpoint; }
};

class ControlThread final : public Event::PrecisionEventThread {
    private:
        int _setpoint = 0;

    protected:
        void OnIteration(
            IterationTime delta,
            IterationTime startTime,
            Threads::SkippedIterationCount skippedIterations
        ) override {
            (void)delta;
            (void)startTime;

            Serial.printf(
                "control setpoint=%d skipped=%llu\n",
                _setpoint,
                static_cast<unsigned long long>(skippedIterations)
            );
        }

    public:
        void ApplySetpoint(SetpointEvent* event) {
            _setpoint = event->GetSetpoint();
        }
};

ControlThread controlThread;
Event::IEventListenerHandle* setpointListener = nullptr;

void setup() {
    Serial.begin(115200);

    controlThread.SetIterationPeriod(
        Units::MilliSeconds<uint64_t>(10)
    );
    controlThread.SetEventProcessOrder(
        Event::PrecisionEventProcessOrder::EventsBeforeIteration
    );
    controlThread.SetEventArrivalPolicy(
        Event::PrecisionEventArrivalPolicy::ProcessOnNextIteration
    );

    setpointListener = controlThread.RegisterListener<SetpointEvent>(
        [](SetpointEvent* event, Event::EventDispatchMethod, Event::EventPriority) {
            controlThread.ApplySetpoint(event);
        }
    );

    Threads::ThreadManager::GetInstance()->Initialize();
}

void loop() {
    static int nextSetpoint = 1;
    (new SetpointEvent(nextSetpoint++))->Queue();
    delay(1000);
}
