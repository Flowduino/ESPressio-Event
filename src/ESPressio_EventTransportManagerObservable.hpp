#pragma once

#include <memory>
#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IEventTransportManagerObserver.hpp"

namespace ESPressio::Event {

class EventTransportManagerObservable final : public Observable::ThreadSafeObservable {
public:
    template<typename TCallback>
    void Notify(TCallback&& callback) {
        ExecuteNotification([&](NotificationContext& notification) {
            notification.WithObservers<IEventTransportManagerObserver>(
                [&](IEventTransportManagerObserver* observer) {
                    try { callback(observer); } catch (...) {}
                });
        });
    }
};

inline std::shared_ptr<EventTransportManagerObservable> CreateEventTransportManagerObservable() {
    return std::make_shared<EventTransportManagerObservable>();
}

}
