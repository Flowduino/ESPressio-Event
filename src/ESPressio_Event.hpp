#pragma once

#include <atomic>
#include <cstdint>

#include <ESPressio_SystemClock.hpp>
#include <ESPressio_TimeTraits.hpp>
#include <ESPressio_ThreadSafe.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventObserver.hpp"
#include "ESPressio_EventManager.hpp"

namespace ESPressio {

    namespace Event {

        /*
         * Generic Event implementation.
         *
         * TTime controls only the public representation of Event lifecycle
         * timestamps. Internally the Event engine stores raw nanoseconds.
         */
        template<
            typename TTime = Timing::DefaultClockTime
        >
        class Event :
            public IEvent {

            private:
                struct DispatchState {
                    bool WasDispatched = false;
                    uint64_t DispatchTimeNanoseconds = 0;

                    bool operator==(
                        const DispatchState& other
                    ) const {
                        return
                            WasDispatched ==
                                other.WasDispatched &&
                            DispatchTimeNanoseconds ==
                                other.DispatchTimeNanoseconds;
                    }
                };


                mutable Threads::ReadWriteMutex<
                    DispatchState
                > _dispatchState{
                    DispatchState()
                };

                std::atomic<uint32_t>
                    _refCount{0};


                static uint64_t
                GetResolutionNanoseconds() {
                    auto& clock =
                        Timing::SystemClock<
                            TTime
                        >::GetInstance();

                    uint64_t resolution =
                        Timing::TimeTraits<
                            TTime
                        >::template
                            ToNanoseconds<
                                uint64_t
                            >(
                                clock.
                                    GetResolution()
                            );

                    return
                        resolution == 0
                            ? 1
                            : resolution;
                }


                static uint64_t
                GetNowNanoseconds() {
                    auto& clock =
                        Timing::SystemClock<
                            TTime
                        >::GetInstance();

                    return
                        Timing::TimeTraits<
                            TTime
                        >::template
                            ToNanoseconds<
                                uint64_t
                            >(
                                clock.GetTime()
                            );
                }


                static TTime
                CreateTime(
                    uint64_t nanoseconds
                ) {
                    const uint64_t resolution =
                        GetResolutionNanoseconds();

                    return
                        Timing::TimeTraits<
                            TTime
                        >::template
                            FromNanoseconds<
                                uint64_t
                            >(
                                nanoseconds,
                                resolution
                            );
                }


            public:
                using TimeType = TTime;


                virtual ~Event() = default;


                void __ref() noexcept override {
                    _refCount.fetch_add(
                        1,
                        std::memory_order_relaxed
                    );
                }


                void __unref() noexcept override {
                    if (
                        _refCount.fetch_sub(
                            1,
                            std::memory_order_acq_rel
                        ) == 1
                    ) {
                        delete this;
                    }
                }


                void __dispatch() override {
                    const uint64_t now =
                        GetNowNanoseconds();

                    _dispatchState.
                        WithWriteLock(
                            [now](
                                DispatchState&
                                    state
                            ) {
                                if (
                                    !state.
                                        WasDispatched
                                ) {
                                    state.
                                        WasDispatched =
                                            true;

                                    state.
                                        DispatchTimeNanoseconds =
                                            now;
                                }
                            }
                        );
                }


                void Queue(
                    EventPriority priority =
                        EventPriority::Normal
                ) override {
                    EventManager::
                        GetInstance()->
                        QueueEvent(
                            this,
                            priority
                        );
                }


                void Stack(
                    EventPriority priority =
                        EventPriority::Normal
                ) override {
                    EventManager::
                        GetInstance()->
                        StackEvent(
                            this,
                            priority
                        );
                }


                uint64_t
                GetDispatchTimeNanoseconds()
                    const override {

                    const DispatchState state =
                        _dispatchState.Get();

                    return
                        state.WasDispatched
                            ? state.
                                DispatchTimeNanoseconds
                            : 0;
                }


                uint64_t
                GetTimeSinceDispatchNanoseconds()
                    const override {

                    const DispatchState state =
                        _dispatchState.Get();

                    if (!state.WasDispatched) {
                        return 0;
                    }

                    const uint64_t now =
                        GetNowNanoseconds();

                    return
                        now >=
                            state.
                                DispatchTimeNanoseconds
                            ? now -
                                state.
                                    DispatchTimeNanoseconds
                            : 0;
                }


                TTime GetDispatchTime() const {
                    return
                        CreateTime(
                            GetDispatchTimeNanoseconds()
                        );
                }


                TTime GetTimeSinceDispatch() const {
                    return
                        CreateTime(
                            GetTimeSinceDispatchNanoseconds()
                        );
                }
        };

    }

}
