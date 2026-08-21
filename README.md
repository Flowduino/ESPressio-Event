# ESPressio Event

Generic Event-Driven Development infrastructure for the Flowduino ESPressio Development Platform.

ESPressio Event provides the asynchronous counterpart to ESPressio Observable: producers dispatch strongly typed data contracts without knowing which consumers exist, while listeners process those Events independently on Event-aware Threads.

## Current Version — 6.0.0

Event 6.0.0 establishes a strict **mechanism-only** dependency boundary. Event owns asynchronous Event lifecycle, dispatch, listeners, Event-aware Threads, transport-neutral Serializable Event routing, and the generic Event Transport abstraction. Concrete Event integrations for ESPressio domains that Event does not otherwise consume now live in the libraries that own those domains.

The core Event programming model remains intact from the 5.x generation; 6.0.0 is a major release because several integration headers moved to their correct owning packages.

# Why Event-Driven Development?

Event-driven design is useful when the producer of an operation should not depend on the implementation, execution context, or even existence of its consumers.

```text
Producer
   |
   | dispatches
   v
 Event<T>
   |
   v
EventManager
   |
   +----------+----------+
   |          |          |
   v          v          v
Listener A Listener B Listener C
```

The Event itself is the **data contract**. The producer populates that contract; listeners read it. The producer never needs to retain references to the listeners.

This gives application modules a clean way to communicate while preserving one-way dependencies and independent scheduling.

## Events are asynchronous

`Queue()` and `Stack()` do not wait for every listener to finish. The dispatching execution path continues while interested Event Threads process the Event independently.

In practical terms, ordinary Events are **fire and forget**.

Use this deliberately: if the caller must synchronously know the result before it can continue, direct sequencing or ESPressio Observable may be the more accurate abstraction.

## No guaranteed global listener order

Independent listeners do not have a meaningful globally guaranteed execution order. Event-driven design should not be used to hide an operation that actually requires strict synchronous sequencing.

## Reciprocal Events

Asynchronous operation still supports request/result workflows. A listener processing one Event can dispatch another Event containing its result:

```text
RequestEvent
     |
     v
 Worker
     |
     v
ResultEvent
```

The requester can listen for `ResultEvent` without acquiring a direct reference to the worker.

## Event vs Observable

The two patterns are complementary:

```text
Observable
    synchronous notification
    callback belongs to the current operation

Event
    asynchronous notification
    producer and consumer should be independently scheduled
```

Timing and Threads Event bridges demonstrate this relationship: those libraries expose synchronous Observable lifecycle callbacks, while Event can optionally translate them into asynchronous Events without making Timing or Threads depend back on Event.

# ESPressio Development Platform

ESPressio libraries are designed to be light-weight, strongly typed, object-oriented, composable, and to follow SOLID dependency boundaries wherever practical on embedded C++ targets.

Event follows those goals by making the Event contract shared while keeping producers, consumers and concrete transports independent.

## License

Licensed under the Apache License 2.0. See [LICENSE](LICENSE).

# Namespace

```cpp
ESPressio::Event
```

Important public concepts include:

- `IEvent` — type-erased Event interface used by routing infrastructure.
- `Event<TTime>` — normal strongly typed Event base.
- `EventManager` — central dispatch/routing manager.
- `EventListener` / listener handles — type-specific consumer registration.
- `EventThread` — asynchronous Event-processing Thread.
- `PrecisionEventThread` — deterministic periodic execution combined with Event processing.
- `EventPriority` and `EventDispatchMethod`.
- `EventTransportManager` and `IEventTransport`.
- optional Serializable Event registration/discovery/construction.

# Dependencies

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

Event no longer consumes Command, Security, Sockets or ESP-Now merely to host their domain-specific bridges.

See [DEPENDENCY_BOUNDARIES.md](DEPENDENCY_BOUNDARIES.md) and [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md).

# Installation

PlatformIO:

```ini
lib_deps =
    flowduino/ESPressio-Event@^6.0.0
```

For Serializable Events/Event Transport, also include:

```ini
lib_deps =
    flowduino/ESPressio-Serializable@^0.10.2
```

The current ESP32 implementation uses C++17 and RTTI. When the surrounding toolchain disables RTTI, enable it in the project configuration.

# Defining and dispatching a basic Event

An Event should contain the immutable contextual data that consumers need.

```cpp
#include <ESPressio_Event.hpp>

class TemperatureChangedEvent final :
    public ESPressio::Event::Event<> {
private:
    const float _previous;
    const float _current;

public:
    TemperatureChangedEvent(float previous, float current) :
        _previous(previous),
        _current(current) {}

    float GetPrevious() const { return _previous; }
    float GetCurrent() const { return _current; }
};
```

Dispatch FIFO-style with `Queue()`:

```cpp
(new TemperatureChangedEvent(21.0f, 21.5f))->Queue();
```

or LIFO-style with `Stack()`:

```cpp
(new TemperatureChangedEvent(21.0f, 21.5f))->Stack();
```

Once dispatched, application code should treat an Event as immutable and should not retain ownership of the raw pointer. The Event infrastructure manages its lifecycle while interested receivers process it.

# Listening for Events

Listeners are registered against an Event-aware Thread and return an owning listener handle.

A representative pattern from the current `PrecisionEventThread` example is:

```cpp
#include <ESPressio_Event.hpp>
#include <ESPressio_PrecisionEventThread.hpp>
#include <ESPressio_ThreadManager.hpp>

using namespace ESPressio;

class SetpointEvent final : public Event::Event<> {
private:
    const int _setpoint;

public:
    explicit SetpointEvent(int setpoint) : _setpoint(setpoint) {}
    int GetSetpoint() const { return _setpoint; }
};

class ControlThread final : public Event::PrecisionEventThread<> {
private:
    int _setpoint = 0;

protected:
    void OnIteration(
        IterationTime,
        IterationTime,
        Threads::SkippedIterationCount
    ) override {
        // Periodic control work uses the most recently received setpoint.
    }

public:
    void ApplySetpoint(SetpointEvent* event) {
        _setpoint = event->GetSetpoint();
    }
};

ControlThread controlThread;
Event::EventListenerHandlePtr setpointListener;

void setup() {
    controlThread.SetIterationPeriod(
        Units::MilliSeconds<uint64_t>(10)
    );

    setpointListener =
        controlThread.RegisterListener<SetpointEvent>(
            [](SetpointEvent* event,
               Event::EventDispatchMethod,
               Event::EventPriority) {
                controlThread.ApplySetpoint(event);
            }
        );

    Threads::ThreadManager::GetInstance()->Initialize();
}
```

Keep the returned `EventListenerHandlePtr` alive for as long as the listener should remain registered.

# `EventThread`

`EventThread` is designed for modules whose work is driven by incoming Events. Unlike an ordinary looping Thread, it can remain suspended efficiently until a relevant Event arrives, process the Events delivered to it, then return to waiting.

An Event can be dispatched from anywhere; only interested Event receivers need to know how it is processed.

This is useful for decomposing an application into independent modules such as:

```text
Sensor module
    -> SensorReadingEvent

Control module
    -> consumes SensorReadingEvent
    -> emits ActuatorTargetEvent

UI / diagnostics module
    -> independently consumes either Event
```

No module needs a concrete reference to the others.

# `PrecisionEventThread`

`PrecisionEventThread` combines periodic deterministic work with Event reception. Applications can choose whether pending Events are processed before/after each iteration and how Events arriving between iteration boundaries should be handled.

See:

```text
examples/PrecisionEventThread/PrecisionEventThread.ino
```

for the current API, including `SetIterationPeriod()`, `SetEventProcessOrder()` and `SetEventArrivalPolicy()`.

# Event lifecycle timing

`Event<TTime>` uses ESPressio Timing for lifecycle timestamps. The default public representation is `Timing::DefaultClockTime`.

```cpp
auto dispatched = event.GetDispatchTime();
auto age = event.GetTimeSinceDispatch();
```

The first dispatch timestamp is retained if the same Event is redispatched.

Type-erased infrastructure also exposes nanosecond timing values so routing internals do not depend on a particular public Unit representation.

# Event priority

Events may be dispatched using the supported `EventPriority` levels. Priority participates in the receiver's normal dispatch ordering. When not supplied explicitly, normal priority is used.

# Bounded Event queues and diagnostics

Event receiver queues are bounded by default so an embedded application cannot grow pending Event storage without limit merely because a consumer falls behind.

The default maximum can be configured with:

```cpp
ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT
```

Queue diagnostics include facilities for inspecting current/peak pending Events and rejected/dropped Events, plus resetting queue statistics.

This makes it possible to distinguish normal Event traffic from sustained backpressure rather than discovering overload only through heap exhaustion.

# Serializable Events

Serializable support is deliberately optional. Local-only Events do not require ESPressio Serializable.

A Serializable Event combines the Event contract with ESPressio Serializable metadata. The repository contains a complete example at:

```text
examples/SerializableEvent/
```

Event Transport uses the existing EVTT transport envelope and ESPB v2 Serializable payload representation; Event 6.0.0 did not change the wire format.

# Runtime Serializable Event discovery

The Serializable Event registry can be inspected at runtime without compile-time knowledge of every concrete Event type. This is useful for operator consoles, REST/WebSocket gateways, test harnesses, Event replay and discovery tooling.

```cpp
auto& manager =
    ESPressio::Event::EventTransportManager::GetInstance();

for (const auto& descriptor :
     manager.GetRegisteredSerializableEvents()) {
    // descriptor.TypeID
    // descriptor.TypeName
    // descriptor.SchemaVersion
    // descriptor.Properties
    // descriptor.CanConstruct
}
```

Look up a specific stable wire type:

```cpp
ESPressio::Event::SerializableEventDescriptor descriptor;

if (manager.FindRegisteredSerializableEvent(
        "flowduino.camera.shutter.v1",
        descriptor)) {
    // inspect schema/property metadata
}
```

Descriptors are snapshots; callers do not gain mutable references to Event Transport's private registration table.

# Runtime Serializable Event construction

Event deliberately accepts a representation-neutral `SerializationNode` rather than parsing JSON itself:

```text
JSON / CBOR / CLI / replay data
          |
          v
   SerializationNode
          |
          v
EventTransportManager
          |
          v
concrete Serializable Event
```

A current example:

```cpp
auto result = manager.CreateSerializableEvent(
    "flowduino.example.operator-command.v1",
    payload
);

if (!result) {
    for (const auto& issue : result.Deserialization.Issues()) {
        // issue.Path
        // issue.Message
    }
}
```

The concrete Serializable Event remains responsible for schema migration, aliases, defaults, required properties, numeric constraints, validators and detailed deserialization diagnostics.

A successfully constructed type-erased Event can be dispatched without recovering its compile-time C++ type:

```cpp
ESPressio::Event::EventTransportManager::DispatchSerializableEvent(
    std::move(result.Event),
    ESPressio::Event::EventDispatchMethod::Queue,
    ESPressio::Event::EventPriority::Normal
);
```

See the complete current example:

```text
examples/RuntimeSerializableEvents/RuntimeSerializableEvents.ino
```

# Event Transport

Event owns the transport-neutral contract:

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

Concrete transports are provided by libraries such as ESPressio ESP-Now and ESPressio Sockets. Event itself does not need to know whether an Event is carried over ESP-NOW, UDP, TCP, TLS, WebSocket, MQTT or another future transport.

## Registration and routing

An application registers Serializable Event types with `EventTransportManager` and registers one or more concrete `IEventTransport` implementations. Direction/routing policy then determines which locally dispatched Events are handed to transports and which validated inbound Events are reconstructed for local dispatch.

The repository contains current transport examples for:

```text
EventTransportLoopback
EventTransportMultiTransportRouting
```

These are useful references when implementing or testing a concrete transport.

# Timing/SystemClock Event bridge

Timing is a required upstream dependency of Event, so its Observer-to-Event bridge correctly lives in Event without introducing a reciprocal dependency.

```cpp
#include <ESPressio_SystemClockEventBridge.hpp>
```

A Serializable counterpart is also available when Serializable Event transport of the Timing lifecycle is required:

```cpp
#include <ESPressio_SystemClockEventBridge_Serializable.hpp>
```

See:

```text
examples/SystemClockEventBridge/
```

# Threads infrastructure Event bridges

Threads is also a required upstream dependency of Event. Event therefore supplies the asynchronous representation of relevant Threads lifecycle observations:

```cpp
#include <ESPressio_ThreadEventBridges.hpp>
```

and, where needed:

```cpp
#include <ESPressio_ThreadEventBridges_Serializable.hpp>
```

See:

```text
examples/ThreadInfrastructureEventBridges/
```

# Domain Event integrations moved in 6.0.0

Event does **not** own concrete Event representations for ESPressio Command, Security, Sockets or ESP-Now. Those integrations now live with their owning libraries:

```text
Command 0.4.0
    Command Event types
    CommandRegistryEventBridge

Security 0.3.0
    TransportSecurity Event types
    TransportSecurityEventBridge

Sockets 0.6.0
    Socket Event types
    SocketWorkerEventBridge
    SocketSecuritySessionEventBridge

ESP-Now 0.6.0
    ESPNow Event types
    ESPNowTransportEventBridge
    ESPNowEventTransport
```

Where names remained semantically correct, public header/class names were preserved. Applications simply obtain them from the correct domain package.

The intended dependency direction is one-way:

```text
Command  - - -> Event
Security - - -> Event
Sockets  - - -> Event
ESP-Now  - - -> Event
```

Event CI enforces that the reverse edges do not return.

# Design guidance

Event-driven design is particularly effective when:

- application modules should be independently scheduled;
- producers should not know how many consumers exist;
- new consumers should be addable without modifying producers;
- work can be represented as immutable data contracts; and
- request/result flows can naturally use reciprocal Events.

Prefer synchronous Observable/direct calls when the producer really does require immediate ordered completion from the consumer.

# Examples

Current examples include:

```text
examples/EventTransportLoopback/
examples/EventTransportMultiTransportRouting/
examples/PrecisionEventThread/
examples/RuntimeSerializableEvents/
examples/SerializableEvent/
examples/SystemClockEventBridge/
examples/ThreadInfrastructureEventBridges/
```

They are compiled against current `main` in CI and are the best source for complete application-shaped usage.

# Compatibility

ESPressio Event targets ESP32-family microcontrollers using Arduino-ESP32. The current architecture uses ESP-IDF FreeRTOS facilities, C++17, RTTI, ESPressio Threads/Timing/Observable, and optional Serializable support.

Event 6.0.0 does not change core dispatch semantics, Event listener/receiver semantics, lifecycle timestamps, Serializable payload representation, Event Transport envelope format, or routing/origin/message-ID/hop semantics.

# Changelog

See [CHANGELOG.md](CHANGELOG.md) for release-by-release history.
