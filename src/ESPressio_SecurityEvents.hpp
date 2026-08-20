#pragma once

#include <cstdint>

#include <ESPressio_SecurityTypes.hpp>

#include "ESPressio_Event.hpp"

namespace ESPressio::Event {

class TransportSecurityConfigurationChangedEvent final : public Event<> {
public:
    const Security::TransportSecurityConfig Before;
    const Security::TransportSecurityConfig After;
    TransportSecurityConfigurationChangedEvent(
        const Security::TransportSecurityConfig& before,
        const Security::TransportSecurityConfig& after
    ) : Before(before), After(after) {}
};

class TransportSecuritySessionResetEvent final : public Event<> {
public:
    const uint64_t PreviousSessionID;
    explicit TransportSecuritySessionResetEvent(uint64_t previousSessionID)
        : PreviousSessionID(previousSessionID) {}
};

class TransportSecuritySessionEstablishedEvent final : public Event<> {
public:
    const uint64_t SessionID;
    explicit TransportSecuritySessionEstablishedEvent(uint64_t sessionID)
        : SessionID(sessionID) {}
};

class TransportSecurityReplayProtectionResetEvent final : public Event<> {};

class TransportSecurityFailureEvent final : public Event<> {
public:
    const Security::SecurityResult Result;
    explicit TransportSecurityFailureEvent(const Security::SecurityResult& result)
        : Result(result) {}
};

} // namespace ESPressio::Event
