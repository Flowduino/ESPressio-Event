#pragma once

#include <atomic>
#include <cstdint>

#include <ESPressio_SystemClock.hpp>
#include <ESPressio_TimeTraits.hpp>

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
                /*
                 * Event instances are intentionally short-lived, so lifecycle
                 * metadata must not own pthread/FreeRTOS lock resources.
                 *
                 * 64-bit values are stored as two 32-bit atomics so this path
                 * does not depend on a target-specific 64-bit atomic lock.
                 */
                static uint64_t CombineUInt64(
                    uint32_t low,
                    uint32_t high
                ) noexcept {
                    return
                        static_cast<uint64_t>(low) |
                        (static_cast<uint64_t>(high) << 32u);
                }


                static uint32_t LowUInt32(
                    uint64_t value
                ) noexcept {
                    return static_cast<uint32_t>(value);
                }


                static uint32_t HighUInt32(
                    uint64_t value
                ) noexcept {
                    return static_cast<uint32_t>(value >> 32u);
                }


                std::atomic<uint32_t>
                    _refCount{0};

                /*
                 * Dispatch state:
                 *   0 = not dispatched
                 *   1 = timestamp being published
                 *   2 = dispatched/timestamp ready
                 */
                std::atomic<uint8_t>
                    _dispatchState{0};

                std::atomic<uint32_t>
                    _dispatchTimeLow{0};

                std::atomic<uint32_t>
                    _dispatchTimeHigh{0};

                /*
                 * EventDispatchContext is published with a tiny sequence lock.
                 * Every field is itself atomic, so readers/writers remain
                 * data-race-free while the sequence guarantees one coherent
                 * snapshot.
                 */
                mutable std::atomic<uint32_t>
                    _dispatchContextSequence{0};

                std::atomic<uint8_t>
                    _dispatchContextOrigin{
                        static_cast<uint8_t>(
                            EventOrigin::Local
                        )
                    };

                std::atomic<uint32_t>
                    _dispatchContextMessageLow{0};

                std::atomic<uint32_t>
                    _dispatchContextMessageHigh{0};

                std::atomic<uint8_t>
                    _dispatchContextHopCount{0};


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


                uint64_t
                ReadDispatchTimeNanoseconds() const noexcept {
                    while (
                        _dispatchState.load(
                            std::memory_order_acquire
                        ) == 1
                    ) {
                        /* publication is only two atomic stores */
                    }

                    if (
                        _dispatchState.load(
                            std::memory_order_acquire
                        ) != 2
                    ) {
                        return 0;
                    }

                    const uint32_t low =
                        _dispatchTimeLow.load(
                            std::memory_order_relaxed
                        );

                    const uint32_t high =
                        _dispatchTimeHigh.load(
                            std::memory_order_relaxed
                        );

                    return CombineUInt64(low, high);
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
                    uint32_t current =
                        _refCount.load(
                            std::memory_order_acquire
                        );

                    while (current != 0) {
                        if (
                            _refCount.
                                compare_exchange_weak(
                                    current,
                                    current - 1,
                                    std::memory_order_acq_rel,
                                    std::memory_order_acquire
                                )
                        ) {
                            if (current == 1) {
                                delete this;
                            }

                            return;
                        }
                    }

                    /*
                     * Defensive no-op: an unmatched __unref() must never wrap
                     * the unsigned reference count to UINT32_MAX.
                     */
                }


                void __setDispatchContext(
                    const EventDispatchContext& context
                ) override {
                    _dispatchContextSequence.fetch_add(
                        1,
                        std::memory_order_acq_rel
                    );

                    _dispatchContextOrigin.store(
                        static_cast<uint8_t>(context.Origin),
                        std::memory_order_relaxed
                    );

                    _dispatchContextMessageLow.store(
                        LowUInt32(context.TransportMessageID),
                        std::memory_order_relaxed
                    );

                    _dispatchContextMessageHigh.store(
                        HighUInt32(context.TransportMessageID),
                        std::memory_order_relaxed
                    );

                    _dispatchContextHopCount.store(
                        context.HopCount,
                        std::memory_order_relaxed
                    );

                    _dispatchContextSequence.fetch_add(
                        1,
                        std::memory_order_release
                    );
                }


                EventDispatchContext
                __getDispatchContext() const override {
                    for (;;) {
                        const uint32_t before =
                            _dispatchContextSequence.load(
                                std::memory_order_acquire
                            );

                        if ((before & 1u) != 0u) {
                            continue;
                        }

                        EventDispatchContext result;
                        result.Origin =
                            static_cast<EventOrigin>(
                                _dispatchContextOrigin.load(
                                    std::memory_order_relaxed
                                )
                            );

                        const uint32_t low =
                            _dispatchContextMessageLow.load(
                                std::memory_order_relaxed
                            );

                        const uint32_t high =
                            _dispatchContextMessageHigh.load(
                                std::memory_order_relaxed
                            );

                        result.TransportMessageID =
                            CombineUInt64(low, high);

                        result.HopCount =
                            _dispatchContextHopCount.load(
                                std::memory_order_relaxed
                            );

                        const uint32_t after =
                            _dispatchContextSequence.load(
                                std::memory_order_acquire
                            );

                        if (
                            before == after &&
                            (after & 1u) == 0u
                        ) {
                            return result;
                        }
                    }
                }


                void __dispatch() override {
                    uint8_t expected = 0;

                    if (
                        !_dispatchState.compare_exchange_strong(
                            expected,
                            1,
                            std::memory_order_acq_rel,
                            std::memory_order_acquire
                        )
                    ) {
                        return;
                    }

                    const uint64_t now =
                        GetNowNanoseconds();

                    _dispatchTimeLow.store(
                        LowUInt32(now),
                        std::memory_order_relaxed
                    );

                    _dispatchTimeHigh.store(
                        HighUInt32(now),
                        std::memory_order_relaxed
                    );

                    _dispatchState.store(
                        2,
                        std::memory_order_release
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
                    return ReadDispatchTimeNanoseconds();
                }


                uint64_t
                GetTimeSinceDispatchNanoseconds()
                    const override {

                    const uint64_t dispatchTime =
                        ReadDispatchTimeNanoseconds();

                    if (dispatchTime == 0) {
                        return 0;
                    }

                    const uint64_t now =
                        GetNowNanoseconds();

                    return
                        now >= dispatchTime
                            ? now - dispatchTime
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
