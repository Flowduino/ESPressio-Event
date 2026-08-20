#pragma once

#include <atomic>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <ESPressio_Thread.hpp>
#include "ESPressio_EventReceiver.hpp"

#ifndef ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY
    #define ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY 2
#endif

#ifndef ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID
    #define ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID 0
#endif

using namespace ESPressio::Threads;

namespace ESPressio {

    namespace Event {

        class IEventThreadBase {

        };

        class EventThreadBase : public Thread, public EventReceiver, public IEventThreadBase {
            private:
                std::atomic<TaskHandle_t>
                    _notificationTask{
                        nullptr
                    };

            protected:
                void OnLoop() override {
                    _notificationTask.store(
                        xTaskGetCurrentTaskHandle(),
                        std::memory_order_release
                    );

                    if (GetPendingEventCount() == 0) {
                        ulTaskNotifyTake(
                            pdTRUE,
                            portMAX_DELAY
                        );
                    } else {
                        ulTaskNotifyTake(
                            pdTRUE,
                            0
                        );
                    }

                    WithEvents(
                        [&](
                            IEvent* event,
                            EventDispatchMethod dispatchMethod,
                            EventPriority priority
                        ) {
                            OnEvent(
                                event,
                                dispatchMethod,
                                priority
                            );
                        }
                    );
                }

                virtual void OnEvent(
                    IEvent* event,
                    EventDispatchMethod dispatchMethod,
                    EventPriority priority
                ) = 0;

                void EventAdded() override {
                    const TaskHandle_t task =
                        _notificationTask.load(
                            std::memory_order_acquire
                        );

                    if (task != nullptr) {
                        xTaskNotifyGive(
                            task
                        );
                    }
                }

            public:
                EventThreadBase(bool freeOnTerminate) :
                    Thread(freeOnTerminate) {
                    SetPriority(
                        ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY
                    );
                    SetCoreID(
                        ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID
                    );
                }

                virtual ~EventThreadBase() {
                    _notificationTask.store(
                        nullptr,
                        std::memory_order_release
                    );
                }
        };

    }

}
