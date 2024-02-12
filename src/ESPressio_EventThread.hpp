#pragma once

#include <ESPressio_Thread.hpp>

#include <ESPressio_EventThreadBase.hpp>
#include "ESPressio_EventListener.hpp"
#include "ESPressio_EventManager.hpp"

using namespace ESPressio::Threads;

namespace ESPressio {

    namespace Event {

        class IEventThread {

        };

        class EventThread : public EventThreadBase, public EventListener, public IEventThread {
            protected:
                inline void OnEvent(IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) override {
                    ProcessEvent(event, dispatchMethod, priority);
                }

                void OnListenerRegistered(std::type_index eventType) override {
                    EventManager::GetInstance()->RegisterReceiver(eventType, this);
                }

                void OnListenerUnregistered(std::type_index eventType) override {
                    EventManager::GetInstance()->UnregisterReceiver(eventType, this);
                }
            public:
                virtual String GetThreadNamePrefix() const { return "eventthread"; }

                EventThread(bool freeOnTerminate) : EventThreadBase(freeOnTerminate) { }

                virtual ~EventThread() {

                }
        };

        enum EventThreadProcessOrder {
            EventsBeforeLoop,
            EventsAfterLoop
        };

        class EventThreadWithLoop : public Thread, public EventReceiver, public IEventThreadBase, public EventListener, public IEventThread {
            private:
                EventThreadProcessOrder _processOrder = EventThreadProcessOrder::EventsBeforeLoop;
            protected:
                void OnLoop() override {
                    if (_processOrder == EventThreadProcessOrder::EventsBeforeLoop) {
                        WithEvents([&](IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) {
                            ProcessEvent(event, dispatchMethod, priority);
                        });
                    }

                    OnThreadLoop();

                    if (_processOrder == EventThreadProcessOrder::EventsAfterLoop) {
                        WithEvents([&](IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) {
                            ProcessEvent(event, dispatchMethod, priority);
                        });
                    }
                }

                virtual void OnThreadLoop() = 0;
            public:
                EventThreadWithLoop(bool freeOnTerminate) : Thread(freeOnTerminate) { }

                virtual ~EventThreadWithLoop() {

                }

                EventThreadProcessOrder GetProcessOrder() { return _processOrder; }

                void SetProcessOrder(EventThreadProcessOrder processOrder) { _processOrder = processOrder; }
        };

    }

}