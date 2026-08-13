#pragma once

#include <cstdint>

#include <ESPressio_Clock.hpp>
#include <ESPressio_SystemClock.hpp>
#include <ESPressio_ThreadSafe.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventObserver.hpp"
#include "ESPressio_EventManager.hpp"

using namespace ESPressio::Threads;

namespace ESPressio {

    namespace Event {

        class Event : public IEvent {
            private:
                struct DispatchState {
                    bool wasDispatched = false;
                    EventTime dispatchTime;

                    bool operator==(const DispatchState& other) const {
                        return wasDispatched == other.wasDispatched &&
                            dispatchTime.value == other.dispatchTime.value &&
                            dispatchTime.orderOfMagnitude ==
                                other.dispatchTime.orderOfMagnitude;
                    }
                };

                ReadWriteMutex<DispatchState> _dispatchState =
                    ReadWriteMutex<DispatchState>(DispatchState());
                Mutex<uint32_t> _refCount = Mutex<uint32_t>(0);
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
                    const EventTime now = Timing::SystemClock::
                        GetInstance()->GetTime();
                    _dispatchState.WithWriteLock(
                        [&now](DispatchState& state) {
                            if (!state.wasDispatched) {
                                state.wasDispatched = true;
                                state.dispatchTime = now;
                            }
                        }
                    );
                }

                void Queue(EventPriority priority = EventPriority::Normal) override {
                    EventManager::GetInstance()->QueueEvent(this, priority);
                }

                void Stack(EventPriority priority = EventPriority::Normal) override {
                    EventManager::GetInstance()->StackEvent(this, priority);
                }

                inline EventTime GetDispatchTime() override {
                    return _dispatchState.Get().dispatchTime;
                }

                inline EventTime GetTimeSinceDispatch() override {
                    const EventTime now = Timing::SystemClock::
                        GetInstance()->GetTime();
                    const DispatchState state = _dispatchState.Get();
                    const uint64_t resolution = Timing::ClockBase::
                        GetNanoseconds(
                            Timing::SystemClock::GetInstance()->
                                GetResolution()
                        );

                    if (!state.wasDispatched) {
                        return Timing::ClockBase::CreateClockTime(
                            0, resolution == 0 ? 1 : resolution
                        );
                    }

                    const uint64_t currentNanoseconds =
                        Timing::ClockBase::GetNanoseconds(now);
                    const uint64_t dispatchNanoseconds =
                        Timing::ClockBase::GetNanoseconds(
                            state.dispatchTime
                        );
                    return Timing::ClockBase::CreateClockTime(
                        currentNanoseconds >= dispatchNanoseconds
                            ? currentNanoseconds - dispatchNanoseconds
                            : 0,
                        resolution == 0 ? 1 : resolution
                    );
                }
        };

    }

}
