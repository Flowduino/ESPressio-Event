#pragma once

#include <functional>

namespace ESPressio {

    namespace Event {

        enum EventPriority {
            Low = 0,
            Normal = 1,
            High = 2
        };

        // ++ operator for EventPriority to move to the next value (and roll over to the beginning if at the end)
        inline EventPriority& operator++(EventPriority& priority) {
            priority = static_cast<EventPriority>((static_cast<int>(priority) + 1) % 3);
            return priority;
        }

        // -- operator for EventPriority to move to the previous value (and roll over to the end if at the beginning)
        inline EventPriority& operator--(EventPriority& priority) {
            priority = static_cast<EventPriority>((static_cast<int>(priority) + 2) % 3);
            return priority;
        }

        enum EventListenerInterest {
            All,
            YoungerThan,
            Custom
        };

        // ++ operator for EventListenerInterest to move to the next value (and roll over to the beginning if at the end)
        inline EventListenerInterest& operator++(EventListenerInterest& interest) {
            interest = static_cast<EventListenerInterest>((static_cast<int>(interest) + 1) % 4);
            return interest;
        }

        // -- operator for EventListenerInterest to move to the previous value (and roll over to the end if at the beginning)
        inline EventListenerInterest& operator--(EventListenerInterest& interest) {
            interest = static_cast<EventListenerInterest>((static_cast<int>(interest) + 3) % 4);
            return interest;
        }

        enum EventDispatchMethod {
            Stack,
            Queue
        };

        // ++ operator for EventDispatchMethod to move to the next value (and roll over to the beginning if at the end)
        inline EventDispatchMethod& operator++(EventDispatchMethod& method) {
            method = static_cast<EventDispatchMethod>((static_cast<int>(method) + 1) % 2);
            return method;
        }

        // -- operator for EventDispatchMethod to move to the previous value (and roll over to the end if at the beginning)
        inline EventDispatchMethod& operator--(EventDispatchMethod& method) {
            method = static_cast<EventDispatchMethod>((static_cast<int>(method) + 1) % 2);
            return method;
        }

    }

}

using namespace ESPressio::Event;

// Hash specialization for EventPriority enum
// This enables `EventPriority` to be used as a Key in any set or map type.
namespace std {
    template <>
    struct hash<EventPriority> {
        std::size_t operator()(const EventPriority& priority) const {
            return std::hash<int>()(static_cast<int>(priority));
        }
    };
}