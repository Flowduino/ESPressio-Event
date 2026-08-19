#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "ESPressio_EventEnums.hpp"

namespace ESPressio::Event {

enum class EventTransportDirection : uint8_t {
    None = 0,
    Inbound = 1u << 0,
    Outbound = 1u << 1,
    Bidirectional = (1u << 0) | (1u << 1)
};

constexpr EventTransportDirection operator|(
    EventTransportDirection a,
    EventTransportDirection b
) noexcept {
    return static_cast<EventTransportDirection>(
        static_cast<uint8_t>(a) |
        static_cast<uint8_t>(b)
    );
}

constexpr EventTransportDirection operator&(
    EventTransportDirection a,
    EventTransportDirection b
) noexcept {
    return static_cast<EventTransportDirection>(
        static_cast<uint8_t>(a) &
        static_cast<uint8_t>(b)
    );
}

constexpr EventTransportDirection operator~(
    EventTransportDirection a
) noexcept {
    return static_cast<EventTransportDirection>(
        ~static_cast<uint8_t>(a)
    );
}

constexpr EventTransportDirection RemoveDirection(
    EventTransportDirection value,
    EventTransportDirection remove
) noexcept {
    return static_cast<EventTransportDirection>(
        static_cast<uint8_t>(value) &
        ~static_cast<uint8_t>(remove)
    );
}

constexpr bool HasDirection(
    EventTransportDirection value,
    EventTransportDirection test
) noexcept {
    return (
        static_cast<uint8_t>(value) &
        static_cast<uint8_t>(test)
    ) == static_cast<uint8_t>(test);
}

enum class EventTransportPendingAction : uint8_t {
    Complete,
    Discard
};

struct EventTransportUnregistrationOptions {
    EventTransportPendingAction PendingOutbound =
        EventTransportPendingAction::Complete;

    EventTransportPendingAction PendingInbound =
        EventTransportPendingAction::Complete;
};

enum class EventOrigin : uint8_t {
    Local,
    Remote
};

struct EventDispatchContext {
    EventOrigin Origin = EventOrigin::Local;
    uint64_t TransportMessageID = 0;
    uint8_t HopCount = 0;
};

#pragma pack(push, 1)
struct EventTransportEnvelope {
    static constexpr uint32_t MagicValue =
        0x45565454u; // EVTT

    static constexpr uint8_t CurrentVersion = 1;

    uint32_t Magic = MagicValue;
    uint8_t Version = CurrentVersion;
    uint8_t DispatchMethod =
        static_cast<uint8_t>(
            EventDispatchMethod::Queue
        );

    uint8_t Priority =
        static_cast<uint8_t>(
            EventPriority::Normal
        );

    uint8_t HopCount = 0;
    uint64_t EventTypeID = 0;
    uint32_t SchemaVersion = 1;
    uint64_t MessageID = 0;
    uint32_t PayloadLength = 0;
};
#pragma pack(pop)

static_assert(
    sizeof(EventTransportEnvelope) == 32,
    "EventTransportEnvelope wire layout changed; increment protocol version deliberately."
);

struct EventTransportPacket {
    const uint8_t* Data = nullptr;
    std::size_t Size = 0;
    uint64_t MessageID = 0;
};

enum class EventTransportRegistrationResult : uint8_t {
    Registered,
    Updated,
    AlreadyRegistered,
    TypeConflict,
    InvalidTransport
};

enum class EventTransportUnregistrationResult : uint8_t {
    Updated,
    Removed,
    NotRegistered,
    InvalidTransport
};

struct EventTransportBulkOperationResult {
    std::size_t Requested = 0;
    std::size_t Changed = 0;
    std::size_t Unchanged = 0;
    std::size_t Failed = 0;
};

template<typename TEvent>
struct EventTransportTypeTraits {
    static constexpr std::string_view Name{};
};

constexpr uint64_t EventTransportTypeHash(
    std::string_view value
) noexcept {
    uint64_t hash =
        14695981039346656037ull;

    for (char c : value) {
        hash ^=
            static_cast<uint8_t>(c);

        hash *=
            1099511628211ull;
    }

    return hash;
}

template<typename TEvent>
constexpr uint64_t EventTransportTypeID()
    noexcept {
    return
        EventTransportTypeHash(
            EventTransportTypeTraits<
                TEvent
            >::Name
        );
}

}

#define ESPRESSIO_EVENT_TRANSPORT_TYPE(Type, StableName) \
    namespace ESPressio::Event { \
        template<> struct EventTransportTypeTraits<Type> { \
            static constexpr std::string_view Name = StableName; \
        }; \
    }
