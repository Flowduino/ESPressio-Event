#pragma once

#include <string>

#include "ESPressio_Event.hpp"

namespace ESPressio::Event {

class SocketWorkerStartedEvent final : public Event<> {
public:
    const std::string Name;
    explicit SocketWorkerStartedEvent(const char* name) : Name(name == nullptr ? "" : name) {}
};

class SocketWorkerStartFailedEvent final : public Event<> {
public:
    const std::string Name;
    explicit SocketWorkerStartFailedEvent(const char* name) : Name(name == nullptr ? "" : name) {}
};

class SocketWorkerStoppedEvent final : public Event<> {};

} // namespace ESPressio::Event
