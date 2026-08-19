#include <Arduino.h>

#include <ESPressio_Event.hpp>
#include <ESPressio_EventThread.hpp>

#include <ESPressio_ThreadEventBridges.hpp>
#include <ESPressio_ThreadEvents.hpp>

using namespace ESPressio;

class InfrastructureEventThread final :
    public Event::EventThread {

private:
    Event::EventListenerHandlePtr
        _registeredHandle;

    Event::EventListenerHandlePtr
        _cleanupHandle;

    Event::EventListenerHandlePtr
        _terminationHandle;

public:
    InfrastructureEventThread() :
        Event::EventThread(false) {

        _registeredHandle =
            RegisterListener<
                Event::ThreadRegisteredEvent
            >(
                [](
                    Event::ThreadRegisteredEvent* event,
                    Event::EventDispatchMethod,
                    Event::EventPriority
                ) {
                    Serial.printf(
                        "registered id=%u core=%d\n",
                        event->Snapshot.ThreadID,
                        event->Snapshot.CoreID
                    );
                }
            );

        _cleanupHandle =
            RegisterListener<
                Event::ThreadGarbageCollectionCompletedEvent
            >(
                [](
                    Event::ThreadGarbageCollectionCompletedEvent* event,
                    Event::EventDispatchMethod,
                    Event::EventPriority
                ) {
                    Serial.printf(
                        "GC deleted=%u\n",
                        static_cast<unsigned int>(
                            event->Result.
                                ManagerResult.
                                ThreadsDeleted
                        )
                    );
                }
            );

        _terminationHandle =
            RegisterListener<
                Event::ThreadTerminationDispatchCompletedEvent
            >(
                [](
                    Event::ThreadTerminationDispatchCompletedEvent* event,
                    Event::EventDispatchMethod,
                    Event::EventPriority
                ) {
                    Serial.printf(
                        "termination dispatched id=%u\n",
                        event->Snapshot.ThreadID
                    );
                }
            );
    }
};

InfrastructureEventThread infrastructureEvents;

void setup() {
    Serial.begin(115200);

    Event::ThreadManagerEventBridge::
        GetInstance().
        Initialize();

    Event::ThreadGarbageCollectorEventBridge::
        GetInstance().
        Initialize();

    Event::ThreadTerminationDispatcherEventBridge::
        GetInstance().
        Initialize();

    Threads::ThreadManager::
        GetInstance()->
        Initialize();
}

void loop() {
    delay(1000);
}
