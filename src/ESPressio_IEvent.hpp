#pragma once

#include <cstdint>

#include <ESPressio_IClock.hpp>
#include <ESPressio_ThreadSafe.hpp>

#include "ESPressio_EventEnums.hpp"

using namespace ESPressio::Threads;

#define ESPRESSIO_EVENT_STRICT_THREADSAFE

namespace ESPressio {

    namespace Event {

        using EventTime = Timing::ClockTime;

        class IEvent {
            public:
                virtual ~IEvent() = default;
            // Engine Methods

                /// `__ref` increases the Reference Count for an `IEvent` object.
                /// You should not call this method in your code under normal circumstances.
                virtual void __ref() noexcept = 0; /// Not intended for client use!

                /// `__unref` decreases the Reference Count for an `IEvent` object.
                /// You should not call this method in your code under normal circumstances.
                virtual void __unref() noexcept = 0; /// Not intended for client use!

                /// `__dispatch` is called by the Event Engine to record necessary point-of-dispatch information
                /// Do not call this in your own code.
                virtual void __dispatch() = 0; /// Not intended for client use!

            // Client Methods

                /// `Queue` dispatches the Event through the Central `EventManager`, and places it on the Event Queue
                virtual void Queue(EventPriority priority = EventPriority::Normal) = 0;

                /// `Stack` dispatches the Event through the Central `EventManager` and places it at the top of the Event Stack
                virtual void Stack(EventPriority priority = EventPriority::Normal) = 0;

            // Getters

                /// Returns the System Clock time at which the Event was first dispatched.
                virtual EventTime GetDispatchTime() = 0;

                /// Returns the elapsed System Clock time since first dispatch.
                virtual EventTime GetTimeSinceDispatch() = 0;
        };

    }

}
