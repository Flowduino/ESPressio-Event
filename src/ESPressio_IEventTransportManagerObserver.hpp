#pragma once

#include <ESPressio_IObserver.hpp>

#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_IEventTransport.hpp"

namespace ESPressio::Event {

class IEventTransportManagerObserver :
    public virtual Observable::IObserver {

public:
    virtual ~IEventTransportManagerObserver() = default;

    virtual void OnEventTransportRegistered(
        IEventTransport*
    ) {}

    virtual void OnEventTransportUnregistered(
        IEventTransport*
    ) {}

    virtual void OnEventTransportTypeRegistered(
        uint64_t,
        EventTransportDirection
    ) {}

    virtual void OnEventTransportTypeRegistrationChanged(
        uint64_t,
        EventTransportDirection,
        EventTransportDirection
    ) {}

    virtual void OnEventTransportTypeUnregistered(
        uint64_t,
        EventTransportDirection,
        EventTransportDirection
    ) {}

    virtual void OnEventTransportTypeRouteRegistered(
        uint64_t,
        IEventTransport*,
        EventTransportDirection
    ) {}

    virtual void OnEventTransportTypeRouteChanged(
        uint64_t,
        IEventTransport*,
        EventTransportDirection,
        EventTransportDirection
    ) {}

    virtual void OnEventTransportTypeRouteUnregistered(
        uint64_t,
        IEventTransport*,
        EventTransportDirection,
        EventTransportDirection
    ) {}

    virtual void OnOutboundEventAccepted(
        uint64_t,
        uint64_t
    ) {}

    virtual void OnOutboundEventAcceptedForTransport(
        uint64_t,
        uint64_t,
        IEventTransport*
    ) {}

    virtual void OnOutboundEventHandedToTransport(
        uint64_t,
        uint64_t,
        IEventTransport*,
        bool
    ) {}

    virtual void OnInboundPacketAccepted(
        uint64_t,
        uint64_t,
        IEventTransport*
    ) {}

    virtual void OnInboundPacketRejected(
        uint64_t,
        uint64_t,
        IEventTransport*
    ) {}

    virtual void OnInboundEventDeserialized(
        uint64_t,
        uint64_t
    ) {}

    virtual void OnInboundEventDispatched(
        uint64_t,
        uint64_t
    ) {}
};

}
