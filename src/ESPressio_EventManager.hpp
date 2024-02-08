#pragma once

#include "ESPressio_EventDispatcher.hpp"

namespace ESPressio {

    namespace Event {

        class EventManager : public Thread, public EventDispatcher {
            private:
                SemaphoreHandle_t _semaphore = xSemaphoreCreateBinary();
            protected:
                EventManager() : Thread(true) {
                    Initialize();
                    Start();
                }

                void OnLoop() override {
                    xSemaphoreTake(_semaphore, portMAX_DELAY);
                    DispatchEvents();
                }

                void EventAdded() override {
                    xSemaphoreGive(_semaphore);
                }
            public:
                static EventManager* GetInstance() {
                    static EventManager* instance = new EventManager();
                    return instance;
                }

                virtual ~EventManager() {
                    vSemaphoreDelete(_semaphore);
                }

        };

    }

}