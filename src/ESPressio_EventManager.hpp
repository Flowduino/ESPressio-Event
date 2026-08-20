#pragma once

#include <atomic>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <ESPressio_Thread.hpp>
#include "ESPressio_EventDispatcher.hpp"
#include "ESPressio_EventManagerObservable.hpp"

#ifndef ESPRESSIO_EVENT_MANAGER_PRIORITY
    #define ESPRESSIO_EVENT_MANAGER_PRIORITY 2
#endif

#ifndef ESPRESSIO_EVENT_MANAGER_CORE_ID
    #define ESPRESSIO_EVENT_MANAGER_CORE_ID 0
#endif

using namespace ESPressio::Threads;

namespace ESPressio {

    namespace Event {

        class EventManager : public Thread, public EventDispatcher {
            private:
                std::atomic<TaskHandle_t>
                    _notificationTask{
                        nullptr
                    };

                std::shared_ptr<EventManagerObservable> _observable =
                    CreateEventManagerObservable();

            protected:
                EventManager() : Thread(true) {
                    SetPriority(
                        ESPRESSIO_EVENT_MANAGER_PRIORITY
                    );
                    SetCoreID(
                        ESPRESSIO_EVENT_MANAGER_CORE_ID
                    );
                    Initialize();
                    Start();
                }

                void OnLoop() override {
                    _notificationTask.store(
                        xTaskGetCurrentTaskHandle(),
                        std::memory_order_release
                    );

                    /*
                     * If work arrived before this task published its handle,
                     * the pending-count check prevents a lost wakeup. When work
                     * is already pending, clear a possibly accumulated notify
                     * token without blocking and drain the queue immediately.
                     */
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

                    DispatchEvents();
                }

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

                void OnEventDispatched(
                    IEvent* event,
                    EventDispatchMethod method,
                    EventPriority priority
                ) override {
                    _observable->EventDispatched(
                        event, method, priority,
                        event->__getDispatchContext()
                    );
                }

            public:
                Observable::ObserverHandlePtr RegisterObserver(
                    IEventManagerObserver* observer
                ) {
                    return _observable->RegisterObserver(observer);
                }

                void UnregisterObserver(
                    IEventManagerObserver* observer
                ) {
                    _observable->UnregisterObserver(observer);
                }

                static EventManager* GetInstance() {
                    static EventManager* instance = new EventManager();
                    return instance;
                }

                virtual ~EventManager() {
                    _notificationTask.store(
                        nullptr,
                        std::memory_order_release
                    );
                }

        };

    }

}
