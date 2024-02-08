#pragma once

#include <cstdint>

#include <ESPressio_ThreadSafe.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventManager.hpp"

using namespace ESPressio::Base;
using namespace ESPressio::Threads;

namespace ESPressio {

    namespace Event {

        class Event : public IEvent {
            private:
                ReadWriteMutex<unsigned long> _dispatchTime = ReadWriteMutex<unsigned long>(0);
                Mutex<uint32_t> _refCount = Mutex<uint32_t>(0);
                bool _wasDispatched = false;
            public:
                virtual ~Event() { }

                inline void __ref() override {
                    _refCount.WithWriteLock([](uint32_t& refCount) {
                        refCount++;
                    });
                }

                inline void __unref() override {
                    uint32_t cnt = 99;
                    _refCount.WithWriteLock([&cnt](uint32_t& refCount) {
                        refCount--;
                        cnt = refCount;
                    });
                    if (cnt == 0) { delete this; }
                }

                inline void __dispatch() override {
                    if (_wasDispatched) { return; }
                    _wasDispatched = true;
                    _dispatchTime.WithWriteLock([](unsigned long& dispatchTime) {
                        if (dispatchTime == 0) {
                            dispatchTime = millis();
                        }
                    });
                }

                void Queue(EventPriority priority = EventPriority::Normal) override {
                    EventManager::GetInstance()->QueueEvent(this, priority);
                }

                void Stack(EventPriority priority = EventPriority::Normal) override {
                    EventManager::GetInstance()->StackEvent(this, priority);
                }

                inline unsigned long GetDispatchTime() override {
                    return _dispatchTime.Get();
                }

                inline unsigned long GetTimeSinceDispatch() override {
                    return millis() - _dispatchTime.Get();
                }
        };

    }

}