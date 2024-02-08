#pragma once

#include <cstdint>

#include <ESPressio_ThreadSafe.hpp>

#include "ESPressio_EventEnums.hpp"
#include <ESPressio_Object.hpp>

using namespace ESPressio::Base;
using namespace ESPressio::Threads;

#define ESPRESSIO_EVENT_STRICT_THREADSAFE

namespace ESPressio {

    namespace Event {

        class IEvent {
            public:
            // Engine Methods

                /// `__ref` increases the Reference Count for an `IEvent` object.
                /// You should not call this method in your code under normal circumstances.
                virtual void __ref() = 0; /// Not intended for client use!

                /// `__unref` decreases the Reference Count for an `IEvent` object.
                /// You should not call this method in your code under normal circumstances.
                virtual void __unref() = 0; /// Not intended for client use!

                /// `__dispatch` is called by the Event Engine to record necessary point-of-dispatch information
                /// Do not call this in your own code.
                virtual void __dispatch() = 0; /// Not intended for client use!

            // Client Methods

                /// `Queue` dispatches the Event through the Central `EventManager`, and places it on the Event Queue
                virtual void Queue(EventPriority priority = EventPriority::Normal) = 0;

                /// `Stack` dispatches the Event through the Central `EventManager` and places it at the top of the Event Stack
                virtual void Stack(EventPriority priority = EventPriority::Normal) = 0;

            // Getters

                /// `GetDispatchTime` returns the time at which the Event was dispatched (in milliseconds)
                virtual unsigned long GetDispatchTime() = 0;

                /// `GetTimeSinceDispatch` returns the time since the Event was dispatched (in milliseconds)
                virtual unsigned long GetTimeSinceDispatch() = 0;
        };

    }

}