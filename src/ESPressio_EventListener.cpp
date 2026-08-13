#include "ESPressio_EventListener.hpp"

#include <functional>

namespace ESPressio {

    namespace Event {

        template <typename EventType>
        EventListenerHandlePtr IEventListener::RegisterListener(
            std::function<void(
                IEvent*,
                EventDispatchMethod dispatchMethod,
                EventPriority priority)> callback,
                EventListenerInterest interest,
                EventTime maximumTimeSinceDispatch,
                std::function<bool(IEvent*)> customInterestCallback
        ) {
            return RegisterListener(typeid(EventType), callback, interest, maximumTimeSinceDispatch, customInterestCallback);
        }

        template <typename EventType>
        void IEventListener::UnregisterListener(IEventListenerHandle* handler) {
            UnregisterListener(typeid(EventType), handler);
        }

    }

}
