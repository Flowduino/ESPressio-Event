#pragma once

#include <ESPressio_IObserver.hpp>
#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_IEvent.hpp"

namespace ESPressio::Event {

class IEventManagerObserver : public virtual Observable::IObserver {
public:
    virtual ~IEventManagerObserver() = default;
    virtual void OnEventDispatched(
        IEvent*,
        EventDispatchMethod,
        EventPriority,
        const EventDispatchContext&
    ) {}
};

}
