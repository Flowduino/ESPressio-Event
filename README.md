# ESPressio Event

Generic Event-Driven Development infrastructure for the Flowduino ESPressio Development Platform.

## Current Version — 6.0.0

ESPressio Event **6.0.0** establishes a strict mechanism-only dependency boundary. Event owns asynchronous Event lifecycle, dispatch, listeners, Event-aware Threads, transport-neutral Serializable Event routing, and the generic Event Transport abstraction. Concrete Event integrations for ESPressio domains that Event does not otherwise consume have moved to the libraries that own those domains.

This is intentionally a major release because public integration headers previously supplied by ESPressio Event are no longer shipped by this package.

## Responsibilities

ESPressio Event owns:

- typed `Event<TTime>` lifecycle and dispatch;
- `EventManager`, listeners, receivers, priorities, and bounded queues;
- `EventThread` and `PrecisionEventThread`;
- synchronous observation of Event infrastructure;
- optional Serializable Events and runtime Serializable Event construction;
- transport-neutral `IEventTransport` and `EventTransportManager` routing;
- Timing/SystemClock Event bridges;
- Threads infrastructure Event bridges.

Event does **not** own concrete Event representations for ESPressio Command, Security, Sockets, or ESP-Now.

## Dependency model

Required:

```text
ESPressio Threads >= 3.1.4 < 4.0.0
ESPressio Timing >= 2.2.4 < 3.0.0
ESPressio Observable >= 3.0.1 < 4.0.0
```

Optional Serializable Event / Event Transport support:

```text
ESPressio Serializable >= 0.10.2 < 1.0.0
```

These direct dependencies have been audited against Event's implementation:

- **Threads** is required by Event's asynchronous execution infrastructure.
- **Timing** is required directly by Event lifecycle timestamps through `SystemClock` and `TimeTraits`.
- **Observable** is required directly by Event manager and transport-manager observation surfaces.
- **Serializable** remains opt-in for Serializable Events and Event Transport payloads.

See [DEPENDENCY_BOUNDARIES.md](DEPENDENCY_BOUNDARIES.md) for the enforced architectural rules and [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md) for the complete ecosystem graph.

## Bridge ownership in 6.0.0

The domain-specific integrations introduced during the 5.x line now live with their owning libraries:

```text
ESPressio Command 0.4.0
    CommandRegisteredEvent
    CommandUnregisteredEvent
    CommandRegistryEventBridge

ESPressio Security 0.3.0
    TransportSecurity*Event types
    TransportSecurityEventBridge

ESPressio Sockets 0.6.0
    SocketWorker*Event types
    SocketSecuritySession*Event types
    SocketWorkerEventBridge
    SocketSecuritySessionEventBridge

ESPressio ESP-Now 0.6.0
    ESPNow*Event types
    ESPNowTransportEventBridge
    ESPNowEventTransport
```

Where names remained semantically correct, the public header and class names have been preserved. Applications must obtain those headers from the appropriate owning library rather than from ESPressio Event.

### Why Timing and Threads bridges remain here

Event genuinely consumes Timing and Threads as part of its own mechanism. Event is therefore already the lower-order consumer in those relationships:

```text
Event -> Timing
Event -> Threads
```

Keeping their Observer-to-Event bridges in Event does not introduce a reverse dependency. Moving those bridges into Timing or Threads would create precisely the reciprocal relationship this architecture is designed to prevent.

## Dependency-direction guarantee

Event 6.0.0 CI rejects source or package-level dependencies from Event back to:

```text
ESP-Now
Sockets
Command
Security
```

The intended direction is exclusively:

```text
Command  - - -> Event
Security - - -> Event
Sockets  - - -> Event
ESP-Now  - - -> Event
```

for explicitly selected Event integrations.

## Basic Event

```cpp
#include <ESPressio_Event.hpp>

class TemperatureChangedEvent final :
    public ESPressio::Event::Event<> {
public:
    const float Previous;
    const float Current;

    TemperatureChangedEvent(float previous, float current) :
        Previous(previous),
        Current(current) {}
};

(new TemperatureChangedEvent(21.0f, 21.5f))->Queue();
```

Events are asynchronous. Producers do not need to know which listeners consume an Event.

## Event lifecycle timing

`Event<TTime>` uses ESPressio Timing directly. The default public time representation is `Timing::DefaultClockTime`, while the type-erased Event infrastructure retains nanosecond lifecycle values.

```cpp
auto dispatched = event.GetDispatchTime();
auto age = event.GetTimeSinceDispatch();
```

The first dispatch timestamp is retained if an Event is dispatched again.

## Serializable Events

Serializable support is deliberately optional. Local-only Events do not require ESPressio Serializable.

When Serializable Events or Event Transport are selected, applications add a compatible Serializable 0.x release:

```ini
lib_deps =
    flowduino/ESPressio-Event@^6.0.0
    flowduino/ESPressio-Serializable@^0.10.2
```

Event Transport continues to use the existing EVTT transport envelope and ESPB v2 Serializable payload representation. Event 6.0.0 does not introduce a wire-format change.

## Event Transport

Event owns only the transport-neutral contract:

```text
Serializable Event
       |
       v
EventTransportManager
       |
       v
IEventTransport
       |
       +--> concrete transport supplied downstream
```

Concrete transports are supplied by transport libraries such as ESPressio ESP-Now and ESPressio Sockets.

This means Event does not need to know whether an Event is carried over ESP-NOW, UDP, TCP, WebSocket, MQTT, or another transport.

## Timing and Threads Event bridges

Timing and Threads bridges remain opt-in facilities within Event because those libraries are already required upstream dependencies of Event.

Timing bridge headers include:

```cpp
#include <ESPressio_SystemClockEventBridge.hpp>
#include <ESPressio_SystemClockEventBridge_Serializable.hpp>
```

Threads bridge headers include:

```cpp
#include <ESPressio_ThreadEventBridges.hpp>
#include <ESPressio_ThreadEventBridges_Serializable.hpp>
```

## Migration from Event 5.8.x

Core Event APIs, Event Transport abstractions, Serializable Event semantics, Timing bridges, Threads bridges, and Event wire formats remain available.

The breaking change is integration ownership. If an application previously included one of these headers from Event:

```text
ESPressio_CommandEvents.hpp
ESPressio_CommandRegistryEventBridge.hpp
ESPressio_SecurityEvents.hpp
ESPressio_TransportSecurityEventBridge.hpp
ESPressio_SocketEvents.hpp
ESPressio_SocketWorkerEvents.hpp
ESPressio_SocketSecuritySessionEvents.hpp
ESPressio_SocketWorkerEventBridge.hpp
ESPressio_SocketSecuritySessionEventBridge.hpp
ESPressio_ESPNowEvents.hpp
ESPressio_ESPNowTransportEventBridge.hpp
```

add the appropriate domain library at its new release generation and include the same unambiguous header name from that package.

The final coordinated integration generation is:

```text
Command   0.4.0
Security  0.3.0
Sockets   0.6.0
ESP-Now   0.6.0
Event     6.0.0
Serial    0.6.0
```

The foundational/runtime baselines remain:

```text
Observable    3.0.1
Serializable  0.10.2
Units         0.2.3
Timing        2.2.4
Threads       3.1.4
```

## Installation

PlatformIO:

```ini
lib_deps =
    flowduino/ESPressio-Event@^6.0.0
```

RTTI and C++17 are required by the current ESP32 implementation.

## Compatibility

ESPressio Event targets ESP32-family microcontrollers using Arduino-ESP32.

The 6.0.0 architectural change does not alter:

- Event dispatch semantics;
- Event listener/receiver semantics;
- Event lifecycle timestamps;
- Serializable Event payload representation;
- Event Transport envelope format;
- Event routing, origin, message-ID, or hop semantics.

## License

Licensed under the Apache License 2.0. See [LICENSE](LICENSE).
