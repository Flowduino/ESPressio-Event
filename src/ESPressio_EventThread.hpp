#pragma once

#include <ESPressio_EventThreadBase.hpp>
#include "ESPressio_EventListener.hpp"
#include "ESPressio_EventManager.hpp"

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

    }

}