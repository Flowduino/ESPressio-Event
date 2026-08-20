#pragma once

#include <cstddef>
#include <cstdint>

#include <ESPressio_ESPNowTypes.hpp>

#include "ESPressio_Event.hpp"

namespace ESPressio::Event {

class ESPNowTransportInitializedEvent final : public Event<> {};
class ESPNowTransportInitializationFailedEvent final : public Event<> {};
class ESPNowTransportShutdownEvent final : public Event<> {};

class ESPNowPeerAddedEvent final : public Event<> {
public:
    const ESPNow::MacAddress Address;
    explicit ESPNowPeerAddedEvent(const ESPNow::MacAddress& address) : Address(address) {}
};

class ESPNowPeerRemovedEvent final : public Event<> {
public:
    const ESPNow::MacAddress Address;
    explicit ESPNowPeerRemovedEvent(const ESPNow::MacAddress& address) : Address(address) {}
};

class ESPNowSendAcceptedEvent final : public Event<> {
public:
    const ESPNow::MacAddress Destination;
    const uint8_t Protocol;
    const std::size_t PayloadLength;
    ESPNowSendAcceptedEvent(const ESPNow::MacAddress& destination, uint8_t protocol, std::size_t payloadLength)
        : Destination(destination), Protocol(protocol), PayloadLength(payloadLength) {}
};

class ESPNowSendFailedEvent final : public Event<> {
public:
    const ESPNow::MacAddress Destination;
    const uint8_t Protocol;
    const std::size_t PayloadLength;
    ESPNowSendFailedEvent(const ESPNow::MacAddress& destination, uint8_t protocol, std::size_t payloadLength)
        : Destination(destination), Protocol(protocol), PayloadLength(payloadLength) {}
};

} // namespace ESPressio::Event
