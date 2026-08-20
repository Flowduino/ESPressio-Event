# ESPressio Event

Event-driven Observer Pattern components of the Flowduino ESPressio Development Platform.

## Current Development Version

This branch targets **ESPressio Event 5.8.0**.

5.8.0 extends Event's existing Observer-to-Event bridge architecture to the new Observable lifecycle surfaces in ESPressio Security, Command, Sockets and ESP-Now while preserving those libraries' independence from Event.

See [CHANGELOG.md](CHANGELOG.md) for release history.

## Core dependency model

Event 5.8.0 retains the same mandatory ESPressio dependency generation:

- **ESPressio Threads >= 3.1.2 and < 4.0.0**
- **ESPressio Observable >= 3.0.1 and < 4.0.0**
- **ESPressio Timing >= 2.2.2 and < 3.0.0**

Optional Serializable/Event Transport facilities continue to use the current ESPressio Serializable 0.10.x generation.

The new 5.8.0 lifecycle bridges are also optional and require only the corresponding upstream library when that bridge header is selected:

- **Security >= 0.2.0 and < 1.0.0** — `ESPressio_TransportSecurityEventBridge.hpp`
- **Command >= 0.3.0 and < 1.0.0** — `ESPressio_CommandRegistryEventBridge.hpp`
- **Sockets >= 0.5.0 and < 1.0.0** — socket worker/security-session bridges
- **ESP-Now >= 0.5.0 and < 1.0.0** — `ESPressio_ESPNowTransportEventBridge.hpp`

None of those four libraries depends on ESPressio Event merely because the bridges exist.

```text
Threads Observers ----+
Timing Observers -----+
Security Observers ---+
Command Observers ----+--> opt-in Event Bridges --> EventManager
Sockets Observers ----+
ESP-Now Observers ----+
```

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md) for the full platform relationship view.

## Core Event model

The existing Event architecture remains unchanged:

- `Event<TTime>` provides strongly typed public event timing while retaining internal nanosecond lifecycle timing.
- `EventManager` performs asynchronous Queue/Stack dispatch.
- callback listeners and typed Event Observers coexist.
- listener registrations use RAII ownership.
- bounded receiver queues and retained-event limits protect embedded memory.
- Event priority routing and registration snapshots avoid per-dispatch registration copying.

## Event Transport

The transport-neutral distributed Event layer remains available for Serializable Event types through `EventTransportManager` and `IEventTransport`.

Existing capabilities include:

- inbound/outbound/bidirectional Event registration;
- per-transport routing policy;
- stable wire type identities;
- message IDs, origin metadata and hop tracking;
- default loop prevention;
- transaction-stage observation; and
- direct Binary serialization/deserialization with compatibility fallback.

Concrete ESP-NOW, UDP, TCP, TLS, WebSocket and MQTT transports remain implemented by their respective transport libraries rather than by Event itself.

## Existing Timing and Thread bridges

Event already provides opt-in bridges for:

- System Clock observations from ESPressio Timing;
- Thread Manager observations;
- Thread Garbage Collector observations; and
- Thread Termination Dispatcher observations.

Those bridges establish the architectural pattern used by 5.8.0: the upstream subsystem owns synchronous lifecycle observation; Event optionally converts it into asynchronous Events.

## Security bridge

```cpp
#include <ESPressio_TransportSecurityEventBridge.hpp>

ESPressio::Event::TransportSecurityEventBridge bridge;
bridge.Initialize(security);
```

The bridge binds to a specific `Security::TransportSecurity` instance and emits:

- `TransportSecurityConfigurationChangedEvent`
- `TransportSecuritySessionResetEvent`
- `TransportSecuritySessionEstablishedEvent`
- `TransportSecurityReplayProtectionResetEvent`
- `TransportSecurityFailureEvent`

The events copy only ordinary lifecycle/result metadata. Key material is not exposed.

## Command registry bridge

```cpp
#include <ESPressio_CommandRegistryEventBridge.hpp>

ESPressio::Event::CommandRegistryEventBridge::GetInstance().Initialize();
```

The bridge consumes `ICommandRegistryObserver` and emits:

- `CommandRegisteredEvent`
- `CommandUnregisteredEvent`

Command execution itself remains on Command's callback/middleware path and is not duplicated by this bridge.

## Socket bridges

Sockets exposes multiple independently owned runtime objects, so Event keeps their bridges instance-specific.

```cpp
#include <ESPressio_SocketWorkerEventBridge.hpp>
#include <ESPressio_SocketSecuritySessionEventBridge.hpp>

ESPressio::Event::SocketWorkerEventBridge workerBridge;
workerBridge.Initialize(worker);

ESPressio::Event::SocketSecuritySessionEventBridge sessionBridge;
sessionBridge.Initialize(session);
```

The worker bridge emits start/start-failure/stop Events. The secure-session bridge emits fault/reset Events.

## ESP-Now bridge

```cpp
#include <ESPressio_ESPNowTransportEventBridge.hpp>

ESPressio::Event::ESPNowTransportEventBridge::GetInstance().Initialize();
```

The bridge emits Events for:

- transport initialization success/failure;
- shutdown;
- peer addition/removal; and
- ESP-NOW send acceptance/failure.

Inbound application payload delivery remains the ESP-Now protocol-handler responsibility rather than being duplicated through the lifecycle bridge.

## Why bridges live in Event

The dependency direction is deliberate:

```text
Subsystem ---> Observable callback
                 |
                 +--> application/diagnostics
                 +--> Serial monitor
                 +--> Event bridge (only if Event is selected)
```

Putting bridge implementations in Event avoids making a lower-level subsystem acquire Event as a dependency solely to expose facts it already reports synchronously.

## Serializable bridge policy

Not every lifecycle Event should automatically become a distributed Serializable Event. The 5.8.0 bridge Events are local operational/lifecycle facts by default. Serializable variants should be introduced only where a stable, transport-worthy schema is justified rather than mechanically serializing internal diagnostics.

This follows the existing ESPressio principle of keeping distributed wire contracts explicit.

## PlatformIO

Core Event:

```ini
lib_deps =
    flowduino/ESPressio-Event@^5.8.0
    flowduino/ESPressio-Threads@^3.1.2
    flowduino/ESPressio-Observable@^3.0.1
    flowduino/ESPressio-Timing@^2.2.2
```

Add Security, Command, Sockets or ESP-Now only when compiling the corresponding 5.8.0 bridge.

## Compatibility

5.8.0 is a backward-compatible extension of the 5.7.x architecture. Existing Events, listeners, typed Observers, Event Threads, PrecisionEventThread, Timing bridges, Thread bridges and Event Transport APIs remain supported. The new bridge headers do not change Event's mandatory dependency set.

## License

Apache License 2.0. See [LICENSE](LICENSE).
