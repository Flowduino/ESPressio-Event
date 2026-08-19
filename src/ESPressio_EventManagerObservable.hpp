#pragma once

#include <memory>
#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IEventManagerObserver.hpp"

namespace ESPressio::Event {

class EventManagerObservable final : public Observable::ThreadSafeObservable {
public:
    void EventDispatched(
        IEvent* event,
        EventDispatchMethod method,
        EventPriority priority,
        const EventDispatchContext& context
    ) {
        ExecuteNotification([&](NotificationContext& notification) {
            notification.WithObservers<IEventManagerObserver>([&](IEventManagerObserver* observer) {
                try { observer->OnEventDispatched(event, method, priority, context); }
                catch (...) {}
            });
        });
    }
};

inline std::shared_ptr<EventManagerObservable> CreateEventManagerObservable() {
    return std::make_shared<EventManagerObservable>();
}

}
