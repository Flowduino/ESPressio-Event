#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <ESPressio_Serializable.hpp>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_IEvent.hpp"

namespace ESPressio::Event {

struct SerializableEventDescriptor {
    uint64_t TypeID = 0;
    std::string TypeName;
    uint32_t SchemaVersion = 1;
    EventTransportDirection DefaultDirection = EventTransportDirection::None;
    std::vector<Serializable::PropertySchemaInfo> Properties;
    bool CanConstruct = false;
};

struct SerializableEventConstructionResult {
    std::unique_ptr<IEvent> Event;
    Serializable::DeserializationResult Deserialization;
    bool TypeRegistered = false;
    bool Constructible = false;

    bool Success() const noexcept {
        return TypeRegistered && Constructible && Event != nullptr && Deserialization.Success();
    }
    explicit operator bool() const noexcept { return Success(); }
};

enum class RuntimeEventDispatchResult : uint8_t {
    Dispatched,
    NullEvent,
    UnsupportedMethod
};

} // namespace ESPressio::Event
