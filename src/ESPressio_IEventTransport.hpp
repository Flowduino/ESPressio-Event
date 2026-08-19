#pragma once

#include "ESPressio_EventTransportTypes.hpp"

namespace ESPressio::Event {

class IEventTransport;

class IEventTransportReceiver {
public:
    virtual ~IEventTransportReceiver() = default;
    virtual void ReceiveEventTransportPacket(
        IEventTransport* transport,
        const uint8_t* data,
        std::size_t size
    ) = 0;
};

class IEventTransport {
public:
    virtual ~IEventTransport() = default;
    virtual bool Send(const EventTransportPacket& packet) = 0;
    virtual void SetReceiver(IEventTransportReceiver* receiver) = 0;
};

}
