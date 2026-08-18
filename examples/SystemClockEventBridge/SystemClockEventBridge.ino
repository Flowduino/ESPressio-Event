#include <ESPressio_Event.hpp>
#include <ESPressio_SystemClockEventBridge.hpp>
#include <ESPressio_TimingEvents.hpp>

using namespace ESPressio;

class TimingEventThread final : public Event::EventThread {
private:
    Event::EventListenerHandlePtr _synchronizedListener;

public:
    TimingEventThread() {
        _synchronizedListener =
            RegisterListener<Event::SystemClockSynchronizedEvent>(
                [](Event::SystemClockSynchronizedEvent* event,
                   Event::EventDispatchMethod,
                   Event::EventPriority) {
                    Serial.printf(
                        "Clock synchronized: before=%llu after=%llu immediateDiff=%lld ns\n",
                        static_cast<unsigned long long>(event->ClockBeforeNanoseconds),
                        static_cast<unsigned long long>(event->ClockAfterNanoseconds),
                        static_cast<long long>(event->ImmediateDifferenceNanoseconds)
                    );
                }
            );
    }
};

TimingEventThread timingEvents;

void setup() {
    Serial.begin(115200);

    Event::SystemClockEventBridge::
        GetInstance().
        Initialize();

    Threads::ThreadManager::
        GetInstance()->
        Initialize();
}

void loop() {
    delay(1000);
}
