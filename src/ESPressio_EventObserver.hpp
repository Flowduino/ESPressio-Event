#pragma once

#include <ESPressio_IObserver.hpp>

#include "ESPressio_EventEnums.hpp"

namespace ESPressio {

    namespace Event {

        /// Typed Observer interface for receiving an Event without supplying an
        /// explicit callback. Implementations are non-owning and must remain alive
        /// until their Event Listener Handle has been unregistered.
        template <class EventType>
        class IEventObserver : public virtual Observable::IObserver {
            public:
                virtual ~IEventObserver() = default;

                virtual void OnEvent(
                    EventType* event,
                    EventDispatchMethod dispatchMethod,
                    EventPriority priority
                ) = 0;

                /// Used when RegisterObserver selects EventListenerInterest::Custom.
                virtual bool IsInterestedInEvent(EventType* event) {
                    (void)event;
                    return true;
                }
        };

    }

}
