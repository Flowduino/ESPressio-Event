#pragma once

#include <atomic>

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
            private:
                std::atomic<bool> _acceptingEvents{true};

                void StopReceivingEvents() noexcept {
                    if (!_acceptingEvents.exchange(false)) {
                        return;
                    }
                    StopAcceptingEvents();
                    try { UnregisterAllListeners(); } catch (...) { }
                    ClearPendingEvents();
                }

            protected:
                inline void OnEvent(IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) override {
                    try {
                        ProcessEvent(event, dispatchMethod, priority);
                    } catch (...) {
                        StopReceivingEvents();
                        throw;
                    }
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
                    Shutdown();
                    StopReceivingEvents();
                }

                void Terminate() override {
                    StopReceivingEvents();
                    EventThreadBase::Terminate();
                }
        };

        enum EventThreadProcessOrder {
            EventsBeforeLoop,
            EventsAfterLoop
        };

        class EventThreadWithLoop : public Thread, public EventReceiver, public IEventThreadBase, public EventListener, public IEventThread {
            private:
                EventThreadProcessOrder _processOrder = EventThreadProcessOrder::EventsBeforeLoop;
                std::atomic<bool> _acceptingEvents{true};

                void StopReceivingEvents() noexcept {
                    if (!_acceptingEvents.exchange(false)) {
                        return;
                    }
                    StopAcceptingEvents();
                    try { UnregisterAllListeners(); } catch (...) { }
                    ClearPendingEvents();
                }
            protected:
                void OnLoop() override {
                    try {
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
                    } catch (...) {
                        StopReceivingEvents();
                        throw;
                    }
                }

                virtual void OnThreadLoop() = 0;

                void OnListenerRegistered(std::type_index eventType) override {
                    EventManager::GetInstance()->RegisterReceiver(eventType, this);
                }

                void OnListenerUnregistered(std::type_index eventType) override {
                    EventManager::GetInstance()->UnregisterReceiver(eventType, this);
                }
            public:
                EventThreadWithLoop(bool freeOnTerminate) : Thread(freeOnTerminate) { }

                virtual ~EventThreadWithLoop() {
                    Shutdown();
                    StopReceivingEvents();
                }

                void Terminate() override {
                    StopReceivingEvents();
                    Thread::Terminate();
                }

                EventThreadProcessOrder GetProcessOrder() { return _processOrder; }

                void SetProcessOrder(EventThreadProcessOrder processOrder) { _processOrder = processOrder; }
        };

    }

}
