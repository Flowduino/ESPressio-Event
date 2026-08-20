#pragma once

#include <string>
#include <utility>
#include <vector>

#include "ESPressio_Event.hpp"

namespace ESPressio::Event {

class CommandRegisteredEvent final : public Event<> {
public:
    const std::vector<std::string> Path;
    explicit CommandRegisteredEvent(const std::vector<std::string>& path) : Path(path) {}
};

class CommandUnregisteredEvent final : public Event<> {
public:
    const std::vector<std::string> Path;
    explicit CommandUnregisteredEvent(const std::vector<std::string>& path) : Path(path) {}
};

} // namespace ESPressio::Event
