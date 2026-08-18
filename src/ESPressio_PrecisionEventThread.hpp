#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <typeindex>

#include <ESPressio_PrecisionThread.hpp>

#include "ESPressio_EventListener.hpp"
#include "ESPressio_EventManager.hpp"
#include "ESPressio_EventReceiver.hpp"
#include "ESPressio_EventThread.hpp"

namespace ESPressio {

    namespace Event {

        enum class PrecisionEventProcessOrder :
            uint8_t {
            EventsBeforeIteration,
            EventsAfterIteration
        };


        enum class PrecisionEventArrivalPolicy :
            uint8_t {
            ProcessOnNextIteration,
            TriggerImmediateIteration,
            ProcessImmediately
        };


        template<
            typename TTime =
                Timing::DefaultClockTime,
            typename TRepresentationTraits =
                Threads::
                    PrecisionThreadTraits<
                        TTime
                    >
        >
        class PrecisionEventThread :
            public Threads::
                PrecisionThread<
                    TTime,
                    TRepresentationTraits
                >,
            public EventReceiver,
            public IEventThreadBase,
            public EventListener,
            public IEventThread {

            private:
                using PrecisionThreadBase =
                    Threads::
                        PrecisionThread<
                            TTime,
                            TRepresentationTraits
                        >;

                mutable std::mutex
                    _eventPolicyMutex;

                PrecisionEventProcessOrder
                    _eventProcessOrder =
                        PrecisionEventProcessOrder::
                            EventsBeforeIteration;

                PrecisionEventArrivalPolicy
                    _eventArrivalPolicy =
                        PrecisionEventArrivalPolicy::
                            ProcessOnNextIteration;

                std::atomic<bool>
                    _acceptingEvents{true};


                class LifecycleObserver final :
                    public Threads::
                        IThreadObserver {

                    private:
                        PrecisionEventThread*
                            _owner;

                    public:
                        explicit
                        LifecycleObserver(
                            PrecisionEventThread*
                                owner
                        ) :
                            _owner(owner) {
                        }


                        void
                        OnThreadExecutionFailed(
                            Threads::IThread*,
                            std::exception_ptr
                        ) override {
                            _owner->
                                StopReceivingEvents();
                        }


                        void
                        OnThreadTerminated(
                            Threads::IThread*
                        ) override {
                            _owner->
                                StopReceivingEvents();
                        }
                };


                LifecycleObserver
                    _lifecycleObserver{
                        this
                    };

                Observable::
                    ObserverHandlePtr
                        _lifecycleObserverHandle;


                void StopReceivingEvents()
                    noexcept {
                    if (
                        !_acceptingEvents.
                            exchange(false)
                    ) {
                        return;
                    }

                    StopAcceptingEvents();

                    try {
                        UnregisterAllListeners();
                    } catch (...) {
                    }

                    ClearPendingEvents();
                }


                void ProcessPendingEvents() {
                    WithEvents(
                        [&](
                            IEvent* event,
                            EventDispatchMethod
                                dispatchMethod,
                            EventPriority priority
                        ) {
                            ProcessEvent(
                                event,
                                dispatchMethod,
                                priority
                            );
                        }
                    );
                }


            protected:
                using typename
                    PrecisionThreadBase::
                        IterationTime;

                using typename
                    PrecisionThreadBase::
                        TimeType;

                using typename
                    PrecisionThreadBase::
                        IterationFrequency;

                using typename
                    PrecisionThreadBase::
                        SignedIterationTime;


                void Iterate(
                    IterationTime delta,
                    IterationTime startTime,
                    Threads::
                        SkippedIterationCount
                            skippedIterations
                ) final override {
                    try {
                        PrecisionEventProcessOrder
                            processOrder;

                        {
                            std::lock_guard<
                                std::mutex
                            > lock(
                                _eventPolicyMutex
                            );

                            processOrder =
                                _eventProcessOrder;
                        }

                        if (
                            processOrder ==
                            PrecisionEventProcessOrder::
                                EventsBeforeIteration
                        ) {
                            ProcessPendingEvents();
                        }

                        OnIteration(
                            delta,
                            startTime,
                            skippedIterations
                        );

                        if (
                            processOrder ==
                            PrecisionEventProcessOrder::
                                EventsAfterIteration
                        ) {
                            ProcessPendingEvents();
                        }
                    } catch (...) {
                        StopReceivingEvents();
                        throw;
                    }
                }


                virtual void OnIteration(
                    IterationTime delta,
                    IterationTime startTime,
                    Threads::
                        SkippedIterationCount
                            skippedIterations
                ) = 0;


                void OnWorkWake()
                    final override {
                    try {
                        ProcessPendingEvents();
                    } catch (...) {
                        StopReceivingEvents();
                        throw;
                    }
                }


                void EventAdded()
                    override {
                    PrecisionEventArrivalPolicy
                        arrivalPolicy;

                    {
                        std::lock_guard<
                            std::mutex
                        > lock(
                            _eventPolicyMutex
                        );

                        arrivalPolicy =
                            _eventArrivalPolicy;
                    }

                    if (
                        arrivalPolicy ==
                        PrecisionEventArrivalPolicy::
                            TriggerImmediateIteration
                    ) {
                        this->Bump();
                    } else if (
                        arrivalPolicy ==
                        PrecisionEventArrivalPolicy::
                            ProcessImmediately
                    ) {
                        this->WakeForWork();
                    }
                }


                void OnListenerRegistered(
                    std::type_index eventType
                ) override {
                    EventManager::
                        GetInstance()->
                        RegisterReceiver(
                            eventType,
                            this
                        );
                }


                void OnListenerUnregistered(
                    std::type_index eventType
                ) override {
                    EventManager::
                        GetInstance()->
                        UnregisterReceiver(
                            eventType,
                            this
                        );
                }


            public:
                using ClockType =
                    Timing::
                        ISystemClock<
                            typename
                                PrecisionThreadBase::
                                    IterationTime
                        >;


                explicit
                PrecisionEventThread(
                    ClockType* clock =
                        nullptr
                ) :
                    PrecisionThreadBase(
                        clock
                    ),
                    _lifecycleObserverHandle(
                        this->
                            RegisterThreadObserver(
                                &_lifecycleObserver
                            )
                    ) {
                }


                PrecisionEventThread(
                    bool freeOnTerminate,
                    ClockType* clock =
                        nullptr
                ) :
                    PrecisionThreadBase(
                        freeOnTerminate,
                        clock
                    ),
                    _lifecycleObserverHandle(
                        this->
                            RegisterThreadObserver(
                                &_lifecycleObserver
                            )
                    ) {
                }


                ~PrecisionEventThread()
                    override {
                    this->Shutdown();
                    StopReceivingEvents();
                }


                void Terminate()
                    override {
                    StopReceivingEvents();
                    PrecisionThreadBase::
                        Terminate();
                }


                PrecisionEventProcessOrder
                GetEventProcessOrder()
                    const {
                    std::lock_guard<
                        std::mutex
                    > lock(
                        _eventPolicyMutex
                    );

                    return
                        _eventProcessOrder;
                }


                void SetEventProcessOrder(
                    PrecisionEventProcessOrder
                        processOrder
                ) {
                    std::lock_guard<
                        std::mutex
                    > lock(
                        _eventPolicyMutex
                    );

                    _eventProcessOrder =
                        processOrder;
                }


                PrecisionEventArrivalPolicy
                GetEventArrivalPolicy()
                    const {
                    std::lock_guard<
                        std::mutex
                    > lock(
                        _eventPolicyMutex
                    );

                    return
                        _eventArrivalPolicy;
                }


                void SetEventArrivalPolicy(
                    PrecisionEventArrivalPolicy
                        arrivalPolicy
                ) {
                    std::lock_guard<
                        std::mutex
                    > lock(
                        _eventPolicyMutex
                    );

                    _eventArrivalPolicy =
                        arrivalPolicy;
                }
        };

    }

}
