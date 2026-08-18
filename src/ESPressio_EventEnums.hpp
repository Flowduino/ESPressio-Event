#pragma once

#include <cstddef>
#include <functional>

namespace ESPressio {

    namespace Event {

        enum EventPriority {
            Low = 0,
            Normal = 1,
            High = 2
        };


        inline EventPriority&
        operator++(
            EventPriority& priority
        ) {
            priority =
                static_cast<EventPriority>(
                    (
                        static_cast<int>(
                            priority
                        ) +
                        1
                    ) %
                    3
                );

            return priority;
        }


        inline EventPriority&
        operator--(
            EventPriority& priority
        ) {
            priority =
                static_cast<EventPriority>(
                    (
                        static_cast<int>(
                            priority
                        ) +
                        2
                    ) %
                    3
                );

            return priority;
        }


        enum EventListenerInterest {
            All,
            YoungerThan,
            Custom
        };


        inline EventListenerInterest&
        operator++(
            EventListenerInterest& interest
        ) {
            interest =
                static_cast<EventListenerInterest>(
                    (
                        static_cast<int>(
                            interest
                        ) +
                        1
                    ) %
                    3
                );

            return interest;
        }


        inline EventListenerInterest&
        operator--(
            EventListenerInterest& interest
        ) {
            interest =
                static_cast<EventListenerInterest>(
                    (
                        static_cast<int>(
                            interest
                        ) +
                        2
                    ) %
                    3
                );

            return interest;
        }


        enum EventDispatchMethod {
            Stack,
            Queue
        };


        inline EventDispatchMethod&
        operator++(
            EventDispatchMethod& method
        ) {
            method =
                static_cast<EventDispatchMethod>(
                    (
                        static_cast<int>(
                            method
                        ) +
                        1
                    ) %
                    2
                );

            return method;
        }


        inline EventDispatchMethod&
        operator--(
            EventDispatchMethod& method
        ) {
            method =
                static_cast<EventDispatchMethod>(
                    (
                        static_cast<int>(
                            method
                        ) +
                        1
                    ) %
                    2
                );

            return method;
        }

    }

}


namespace std {

    template<>
    struct hash<
        ESPressio::Event::
            EventPriority
    > {
        std::size_t operator()(
            const ESPressio::Event::
                EventPriority& priority
        ) const noexcept {
            return
                std::hash<int>()(
                    static_cast<int>(
                        priority
                    )
                );
        }
    };

}
