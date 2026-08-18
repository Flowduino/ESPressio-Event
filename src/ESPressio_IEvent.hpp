#pragma once

#include <cstdint>

#include <ESPressio_ClockTypes.hpp>

#include "ESPressio_EventEnums.hpp"

namespace ESPressio {

    namespace Event {

        /*
         * Default Event timing type used by type-erased listener filtering and
         * by Event<> when no explicit representation is selected.
         */
        using EventTime =
            Timing::DefaultClockTime;


        /*
         * Type-erased Event engine contract.
         *
         * Public typed Event time values deliberately do not appear here.
         * Routing, reference ownership, receivers and listeners therefore
         * remain independent of the selected Event<TTime> representation.
         */
        class IEvent {
            public:
                virtual ~IEvent() = default;

                // Engine Methods
                virtual void __ref() noexcept = 0;
                virtual void __unref() noexcept = 0;
                virtual void __dispatch() = 0;

                // Client Methods
                virtual void Queue(
                    EventPriority priority =
                        EventPriority::Normal
                ) = 0;

                virtual void Stack(
                    EventPriority priority =
                        EventPriority::Normal
                ) = 0;

                /*
                 * Type-erased lifecycle timing for Event infrastructure.
                 *
                 * These values are local System Clock nanoseconds and are not
                 * serialized by SerializableEvent.
                 */
                virtual uint64_t
                GetDispatchTimeNanoseconds() const = 0;

                virtual uint64_t
                GetTimeSinceDispatchNanoseconds() const = 0;
        };

    }

}
