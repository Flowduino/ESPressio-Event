#pragma once

/*
 * Optional ESPressio Serializable integration for ESPressio Event.
 *
 * This file is NOT included by ESPressio_Event.hpp and ESPressio Serializable
 * is deliberately not a mandatory dependency of ESPressio Event.
 */

#if !__has_include(<ESPressio_Serializable.hpp>)
    #error "ESPressio_Event_Serializable.hpp requires ESPressio-Serializable. Add espressio-development-platform/ESPressio-Serializable@^0.9.0 to the consuming project."
#endif

#include <ESPressio_Serializable.hpp>
#include <ESPressio_Time_Serializable.hpp>

#include "ESPressio_Event.hpp"

namespace ESPressio {

    namespace Event {

        /*
         * Serializable Event base.
         *
         * TDerived supplies the payload schema through
         * GetSerializableProperties(), exactly as any other ESPressio
         * Serializable type does.
         *
         * The default Event TimeType is itself Serializable.
         *
         * IMPORTANT:
         * Event engine state is intentionally excluded from serialization:
         *
         *   - reference count
         *   - local dispatch state
         *   - local System Clock dispatch timestamp
         *
         * A deserialized Event is therefore a new local Event ready to be
         * dispatched by the receiving process/device.
         */
        template<
            typename TDerived,
            typename TTime =
                Units::
                    SerializableNanoSeconds<
                        uint64_t
                    >
        >
        class SerializableEvent :
            public Event<TTime>,
            public Serializable::
                SerializableBase<
                    TDerived
                > {

            public:
                using TimeType = TTime;
                using EventBase =
                    Event<TTime>;

                virtual ~SerializableEvent() =
                    default;
        };

    }

}
