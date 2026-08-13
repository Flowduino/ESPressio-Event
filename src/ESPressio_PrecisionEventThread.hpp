#pragma once

#include <cstdint>
#include <mutex>
#include <typeindex>

#include <ESPressio_PrecisionThread.hpp>

#include "ESPressio_EventListener.hpp"
#include "ESPressio_EventManager.hpp"
#include "ESPressio_EventReceiver.hpp"
#include "ESPressio_EventThread.hpp"

namespace ESPressio {
    namespace Event {

        enum class PrecisionEventProcessOrder : uint8_t {
            EventsBeforeIteration,
            EventsAfterIteration
        };

        enum class PrecisionEventArrivalPolicy : uint8_t {
            ProcessOnNextIteration,
            TriggerImmediateIteration,
            ProcessImmediately
        };

        class PrecisionEventThread :
            public Threads::PrecisionThread,
            public EventReceiver,
            public IEventThreadBase,
            public EventListener,
            public IEventThread {
            private:
                mutable std::mutex _eventPolicyMutex;
                PrecisionEventProcessOrder _eventProcessOrder =
                    PrecisionEventProcessOrder::EventsBeforeIteration;
                PrecisionEventArrivalPolicy _eventArrivalPolicy =
                    PrecisionEventArrivalPolicy::ProcessOnNextIteration;

                void _processPendingEvents() {
                    WithEvents([&](
                        IEvent* event,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority
                    ) {
                        ProcessEvent(event, dispatchMethod, priority);
                    });
                }

            protected:
                void Iterate(
                    IterationTime delta,
                    IterationTime startTime,
                    Threads::SkippedIterationCount skippedIterations
                ) final override {
                    PrecisionEventProcessOrder processOrder;
                    {
                        std::lock_guard<std::mutex> lock(
                            _eventPolicyMutex
                        );
                        processOrder = _eventProcessOrder;
                    }

                    if (processOrder ==
                        PrecisionEventProcessOrder::EventsBeforeIteration) {
                        _processPendingEvents();
                    }

                    OnIteration(delta, startTime, skippedIterations);

                    if (processOrder ==
                        PrecisionEventProcessOrder::EventsAfterIteration) {
                        _processPendingEvents();
                    }
                }

                virtual void OnIteration(
                    IterationTime delta,
                    IterationTime startTime,
                    Threads::SkippedIterationCount skippedIterations
                ) = 0;

                void OnWorkWake() final override {
                    _processPendingEvents();
                }

                void EventAdded() override {
                    PrecisionEventArrivalPolicy arrivalPolicy;
                    {
                        std::lock_guard<std::mutex> lock(
                            _eventPolicyMutex
                        );
                        arrivalPolicy = _eventArrivalPolicy;
                    }

                    if (arrivalPolicy == PrecisionEventArrivalPolicy::
                        TriggerImmediateIteration) {
                        Bump();
                    } else if (arrivalPolicy == PrecisionEventArrivalPolicy::
                        ProcessImmediately) {
                        WakeForWork();
                    }
                }

                void OnListenerRegistered(
                    std::type_index eventType
                ) override {
                    EventManager::GetInstance()->RegisterReceiver(
                        eventType, this
                    );
                }

                void OnListenerUnregistered(
                    std::type_index eventType
                ) override {
                    EventManager::GetInstance()->UnregisterReceiver(
                        eventType, this
                    );
                }

            public:
                explicit PrecisionEventThread(
                    Timing::ISystemClock* clock = nullptr
                ) : Threads::PrecisionThread(clock) { }

                PrecisionEventThread(
                    bool freeOnTerminate,
                    Timing::ISystemClock* clock = nullptr
                ) : Threads::PrecisionThread(freeOnTerminate, clock) { }

                ~PrecisionEventThread() override {
                    Shutdown();
                    UnregisterAllListeners();
                }

                PrecisionEventProcessOrder GetEventProcessOrder() const {
                    std::lock_guard<std::mutex> lock(_eventPolicyMutex);
                    return _eventProcessOrder;
                }

                void SetEventProcessOrder(
                    PrecisionEventProcessOrder processOrder
                ) {
                    std::lock_guard<std::mutex> lock(_eventPolicyMutex);
                    _eventProcessOrder = processOrder;
                }

                PrecisionEventArrivalPolicy GetEventArrivalPolicy() const {
                    std::lock_guard<std::mutex> lock(_eventPolicyMutex);
                    return _eventArrivalPolicy;
                }

                void SetEventArrivalPolicy(
                    PrecisionEventArrivalPolicy arrivalPolicy
                ) {
                    std::lock_guard<std::mutex> lock(_eventPolicyMutex);
                    _eventArrivalPolicy = arrivalPolicy;
                }
        };

    }
}
