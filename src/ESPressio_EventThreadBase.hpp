#pragma once

#include <ESPressio_Thread.hpp>
#include "ESPressio_EventReceiver.hpp"

using namespace ESPressio::Threads;

namespace ESPressio {

    namespace Event {

        class IEventThreadBase {

        };

        class EventThreadBase : public Thread, public EventReceiver, public IEventThreadBase {
            private:
                SemaphoreHandle_t _semaphore = xSemaphoreCreateBinary();
            protected:
                void OnLoop() override {
                    xSemaphoreTake(_semaphore, portMAX_DELAY);
                    WithEvents([&](IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) {
                        OnEvent(event, dispatchMethod, priority);
                    });
                }

                virtual void OnEvent(IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) = 0;

                void EventAdded() override {
                    xSemaphoreGive(_semaphore);
                }
            public:
                EventThreadBase(bool freeOnTerminate) : Thread(freeOnTerminate) { }

                virtual ~EventThreadBase() {
                    vSemaphoreDelete(_semaphore);
                }
        };

    }

}