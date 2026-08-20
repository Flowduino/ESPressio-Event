#pragma once

#include <ESPressio_SecurityTypes.hpp>

#include "ESPressio_Event.hpp"

namespace ESPressio::Event {

class SocketSecuritySessionFaultedEvent final : public Event<> {
public:
    const Security::SecurityResult Result;
    explicit SocketSecuritySessionFaultedEvent(const Security::SecurityResult& result) : Result(result) {}
};

class SocketSecuritySessionResetEvent final : public Event<> {};

} // namespace ESPressio::Event
