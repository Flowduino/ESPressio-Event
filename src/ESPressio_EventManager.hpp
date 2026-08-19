#pragma once

#include <ESPressio_Thread.hpp>
#include "ESPressio_EventDispatcher.hpp"
#include "ESPressio_EventManagerObservable.hpp"

using namespace ESPressio::Threads;

namespace ESPressio {

    namespace Event {

        class EventManager : public Thread, public EventDispatcher {
            private:
                SemaphoreHandle_t _semaphore = xSemaphoreCreateBinary();
                std::shared_ptr<EventManagerObservable> _observable =
                    CreateEventManagerObservable();
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
                    vSemaphoreDelete(_semaphore);
                }

        };

    }

}