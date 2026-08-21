# ESPressio Event

Event-Driven Observer Pattern components of the Flowduino ESPressio
Development Platform.

ESPressio Event provides a foundation for designing, structuring, and
implementing ESP32 applications using Event-Driven Development (EDD):
asynchronous typed Event routing, Event-aware Threads, bounded receiver
queues, listener/observer registration, priority dispatch,
high-resolution Event timing, optional Observer-to-Event bridges, and
optional transport-neutral Serializable Event routing.

## Current Version — 5.8.4

ESPressio Event **5.8.4** is a dependency-maintenance patch over 5.8.3. It preserves the allocation-free Event lifecycle synchronization fix introduced in 5.8.3 while refreshing the validated ESP-Now bridge baseline to released ESPressio ESP-Now 0.5.3.

Current dependency model:

```text
Required
    ESPressio Threads >= 3.1.4 < 4.0.0
    ESPressio Timing >= 2.2.4 < 3.0.0
    ESPressio Observable >= 3.0.1 < 4.0.0

Optional Serializable Event / Event Transport
    ESPressio Serializable >= 0.10.2 < 1.0.0

Optional observer bridge sources
    ESPressio Security >= 0.2.0 < 1.0.0
    ESPressio Command >= 0.3.0 < 1.0.0
    ESPressio Sockets >= 0.5.0 < 1.0.0
    ESPressio ESP-Now >= 0.5.3 < 1.0.0
```

Security and Command remain valid one-way bridge dependencies: those libraries expose synchronous Observable contracts and do not depend back upon Event.

The dependency audit for 5.8.4 identifies two reciprocal optional relationships that should not be strengthened:

```text
Sockets -> Event       concrete socket Event transports
Event   -> Sockets     SocketWorkerEventBridge / SocketSecuritySessionEventBridge

ESP-Now -> Event       ESPNowEventTransport
Event   -> ESP-Now     ESPNowTransportEventBridge
```

For 5.8.x compatibility the existing bridge headers remain available. The preferred future architecture moves the Sockets-specific and ESP-Now-specific bridges downstream into their respective optional Event integrations, or into dedicated integration packages, leaving ESPressio Event transport-neutral.

For release-by-release history, see [CHANGELOG.md](CHANGELOG.md).

## Compatibility

ESPressio Event targets the **ESP32 family** using Arduino-ESP32.

The implementation uses ESP-IDF FreeRTOS facilities, C++ RTTI,
allocation-free `std::atomic_flag` guards for per-Event lifecycle metadata,
and Arduino APIs through its dependency stack.

RTTI must be enabled:

``` ini
build_unflags =
    -fno-rtti
```

Compatibility should still be verified by compiling for the intended
ESP32 board/core/toolchain combination.

## ESPressio Development Platform

The **ESPressio Development Platform** is a collection of discrete,
sometimes interconnected component libraries developed around a common
design ethos.

The principal objectives are:

-   **Light-weight** --- components should strive to minimise memory
    consumption and operational overhead without sacrificing clarity or
    correctness.
-   **Ease of Use** --- ESPressio components frequently provide
    developer-friendly, strongly typed abstractions over lower-level
    procedural facilities.
-   **Object-Oriented** --- a type for everything, and everything in a
    type.
-   **SOLID** --- to the maximum extent practical within C++, Arduino,
    FreeRTOS, and microcontroller constraints:
    -   **Single Responsibility Principle (SRP)** --- keep components
        small and focused.
    -   **Open/Closed Principle (OCP)** --- prefer extension without
        modification.
    -   **Liskov Substitution Principle (LSP)** --- derived
        implementations should remain substitutable for their
        abstractions.
    -   **Interface Segregation Principle (ISP)** --- prefer focused,
        client-specific interfaces.
    -   **Dependency Inversion Principle (DIP)** --- depend upon
        abstractions rather than concrete implementations.

ESPressio Event follows those principles by making the Event itself the
shared data contract while keeping Event producers and Event consumers
independent of one another.

## Runtime Serializable Event Discovery and Construction (5.6.0)

ESPressio Event 5.6.0 exposes the Serializable Event registry as a safe runtime API. This is intended for operator consoles, REST/WebSocket gateways, test harnesses, Event replay, and other systems that discover Event types by stable wire identity rather than by C++ template type.

Registered Event types can be enumerated:

```cpp
auto& manager =
    ESPressio::Event::EventTransportManager::GetInstance();

for (const auto& event : manager.GetRegisteredSerializableEvents()) {
    // event.TypeID
    // event.TypeName
    // event.SchemaVersion
    // event.DefaultDirection
    // event.Properties
    // event.CanConstruct
}
```

A specific registration can be queried by stable name or ID:

```cpp
ESPressio::Event::SerializableEventDescriptor descriptor;

if (manager.FindRegisteredSerializableEvent(
        "flowduino.camera.shutter.v1",
        descriptor)) {
    // Inspect descriptor.Properties, schema version, routing, etc.
}
```

Descriptors are snapshots. They do not expose references to the manager's private registration table.

### Runtime construction

Runtime construction accepts ESPressio Serializable's representation-neutral `SerializationNode`:

```cpp
auto result = manager.CreateSerializableEvent(
    "flowduino.camera.shutter.v1",
    payloadNode
);

if (!result) {
    for (const auto& issue : result.Deserialization.Issues()) {
        // issue.Code
        // issue.Path
        // issue.Message
    }
}
```

The normal Serializable machinery remains authoritative: schema migration, aliases, defaults, required properties, numeric constraints, validators, and detailed deserialization diagnostics are all applied by the concrete Event type.

Only Event types for which Event Transport has a runtime factory are constructible. In practice, inbound/bidirectional Serializable Events are expected to be default constructible, matching the existing Event Transport reconstruction model.

### Representation-neutral by design

Event 5.6.0 intentionally does **not** parse JSON itself:

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

A Serial console can therefore use `JsonArchive`, while another integration can construct the same `SerializationNode` from a different representation. This keeps ArduinoJson and console-specific policy out of ESPressio Event.

### Ownership-safe runtime dispatch

A successfully constructed Event can be dispatched without recovering its concrete C++ type:

```cpp
ESPressio::Event::EventTransportManager::DispatchSerializableEvent(
    std::move(result.Event),
    ESPressio::Event::EventDispatchMethod::Queue,
    ESPressio::Event::EventPriority::Normal
);
```

`Queue` and `Stack` are supported. Once dispatched, the Event follows the ordinary Event engine path; existing `EventTransportManager` routing therefore continues to determine whether it is also transmitted over ESP-NOW, UDP, TCP, WebSocket, or another registered transport.

See:

```text
examples/RuntimeSerializableEvents/RuntimeSerializableEvents.ino
```

## License

ESPressio and its component libraries are licensed under the **Apache
License 2.0**.

See [LICENSE](LICENSE) for details.

## Namespace

The Event API resides beneath:

``` cpp
ESPressio::Event
```

## ESPressio Library Dependencies

ESPressio is designed as a modular ecosystem of independently useful
libraries, with required dependencies kept explicit and optional
integrations introduced only when the corresponding functionality is
selected.

For a complete overview of required and opt-in relationships, see:

**[ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.md)**

In the dependency chart:

-   **Solid relationships** represent required ESPressio dependencies.
-   **Dashed relationships** represent opt-in dependencies introduced
    only when the corresponding feature, integration, type, or header is
    used.

### Required dependencies

Normal Event applications require only the dependencies declared by the
library:

``` ini
lib_deps =
    flowduino/ESPressio-Event@^5.8.4
```

The mandatory dependency graph is:

``` text
ESPressio Event 5.8.4
    |
    +-- ESPressio Threads >= 3.1.4 < 4.0.0
    |
    +-- ESPressio Observable >= 3.0.1 < 4.0.0
    |
    +-- ESPressio Timing >= 2.2.4 < 3.0.0
```

ESPressio Units 0.2.3 is carried by the Timing dependency stack.

**ESPressio Serializable is not a mandatory Event dependency.** It is
required only when an application explicitly opts into Serializable
Events, runtime Serializable Event construction, or Event Transport. The
validated current baseline is:

```text
ESPressio Serializable >= 0.10.2 < 1.0.0
```

Security, Command, Sockets and ESP-Now remain optional bridge dependencies only. Event 5.8.4 introduces no new mandatory dependency on any of them. The ESP-Now bridge is validated against ESPressio ESP-Now 0.5.3.

### Dependency direction and transport-specific bridges

The general ESPressio rule is that dependency edges cascade downstream. Event owns transport-neutral Event semantics and `IEventTransport`; concrete transport libraries may consume Event to implement transports. A transport-specific Observer-to-Event bridge should therefore live downstream with that transport integration rather than make Event depend back upon the concrete transport.

The current 5.8.x bridge API is retained for compatibility, but the preferred future locations are:

```text
Sockets Event integration
    SocketWorkerEventBridge
    SocketSecuritySessionEventBridge

ESP-Now Event integration
    ESPNowTransportEventBridge
```

Generic bridges for genuinely upstream libraries such as Timing, Threads, Command and Security remain appropriate within Event because those libraries do not depend back on Event.

------------------------------------------------------------------------

# What is Event-Driven Observer Pattern?

Event-Driven Observer Pattern is a means of fully decoupling distinct
areas of application functionality.

Instead of one module directly invoking another, the producer dispatches
an `Event` containing context-specific payload data. Independent
consumers register interest in that Event type and react when it is
dispatched.

Conceptually:

``` text
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

The producer does not need to know whether zero, one, or many listeners
exist.

An Event is therefore best thought of as a **data contract** between
independently implemented pieces of functionality. The producer
populates the contract; interested consumers read it.

A central `EventManager` coordinates delivery to interested Event
receivers. Any Event type can consequently be dispatched from anywhere
in the application without the dispatching code acquiring direct
relationships with its consumers.

This is a natural asynchronous counterpart to the synchronous Observer
Pattern provided by ESPressio Observable.

## Order of execution

Event-Driven Observer Pattern does not imply a globally defined order of
execution between independent listeners.

If an operation requires a strict synchronous ordering relationship,
conventional Observer callbacks or direct sequencing may be more
appropriate for that particular relationship.

Use the mechanism that correctly expresses the semantics of the
operation rather than forcing every interaction through Events.

## Events are asynchronous

Event dispatch is fundamentally asynchronous.

When an Event is queued or stacked, the execution chain that dispatched
it continues without waiting for every interested receiver to finish
processing it.

In practical terms, Events are **fire and forget**.

This is one of the primary reasons Event-driven design is useful for
separating application concerns: the producer neither knows nor waits
for the implementation details of its consumers.

## Reciprocal Events

Asynchronous operation does not prevent request/result workflows.

A listener processing one Event may dispatch a second, reciprocal Event
containing the result of its work:

``` text
RequestEvent
     |
     v
 Worker
     |
     v
ResultEvent
```

The original requester can itself listen for the result Event without
introducing a direct call relationship between the two modules.

## Event-driven and synchronous Observer patterns are complementary

Not every notification should become an Event.

ESPressio deliberately supports both models:

``` text
ESPressio Observable
    -> synchronous observation
    -> useful when the caller/operation and notification are tightly related

ESPressio Event
    -> asynchronous observation
    -> useful when producers and consumers should remain independently scheduled
```

The opt-in Timing and Threads Event Bridges demonstrate this distinction
directly: the originating libraries expose synchronous Observer
notifications, while ESPressio Event can optionally convert those
notifications into asynchronous Events.

Event 5.8 extends the same model to Security, Command, Sockets, and ESP-Now. The 5.8.4 dependency audit additionally distinguishes valid one-way bridges from transport-specific bridges that should ultimately move downstream to avoid reciprocal optional dependency edges.

------------------------------------------------------------------------

# Understanding the components of ESPressio Event

## `IEvent`

`IEvent` is the non-templated, type-erased Event interface used by the
routing infrastructure.

Routing, receivers, listeners, ownership, and Event Manager
infrastructure operate using:

``` cpp
IEvent*
```

Lifecycle timing is available to type-erased infrastructure as raw
nanoseconds:

``` cpp
uint64_t GetDispatchTimeNanoseconds() const;
uint64_t GetTimeSinceDispatchNanoseconds() const;
```

This keeps the Event engine independent of the public Unit
representation chosen by a concrete Event.

## `Event<TTime>`

The normal Event base is:

``` cpp
template<
    typename TTime = Timing::DefaultClockTime
>
class Event;
```

Ordinary Event code therefore uses:

``` cpp
class MyEvent :
    public ESPressio::Event::Event<> {
};
```

An Event normally contains the immutable contextual payload needed by
its consumers.

For example:

``` cpp
class TemperatureChangeEvent final :
    public ESPressio::Event::Event<> {

private:
    const int _previousTemperature;
    const int _newTemperature;

public:
    TemperatureChangeEvent(
        int previousTemperature,
        int newTemperature
    ) :
        _previousTemperature(previousTemperature),
        _newTemperature(newTemperature) {
    }

    int GetPreviousTemperature() const {
        return _previousTemperature;
    }

    int GetNewTemperature() const {
        return _newTemperature;
    }
};
```

The producer and consumers need to agree only on this Event contract.

### Generic time representation

A different Timing-compatible public representation may be selected:

``` cpp
using MyTime = SomeCompatibleTimeType;

class MyEvent :
    public ESPressio::Event::Event<MyTime> {
};
```

The strongly typed lifecycle API is then:

``` cpp
TTime GetDispatchTime() const;
TTime GetTimeSinceDispatch() const;
```

Internally, Event lifecycle timing remains stored as raw nanoseconds and
is converted through:

``` cpp
Timing::TimeTraits<TTime>
```

The Event System Clock uses:

``` cpp
Timing::SystemClock<TTime>::GetInstance()
```

Timing 2.x typed System Clock facades share one underlying global System
Clock core.

## Dispatching Events

An Event can be dispatched from anywhere.

Queue dispatch provides FIFO semantics:

``` cpp
(new TemperatureChangeEvent(
    previousTemperature,
    temperature
))->Queue();
```

Stack dispatch provides LIFO semantics:

``` cpp
(new TemperatureChangeEvent(
    previousTemperature,
    temperature
))->Stack();
```

The first dispatch records the Event's dispatch time. Redispatching the
same Event does not replace that original timestamp.

## Event priority

Events may be dispatched using the supported Event priority levels.
Priority participates in the Event receiver's normal dispatch ordering.

When no explicit priority is supplied, normal priority is used.

## Event receiver queues

Event receiver queues are bounded to 64 pending Events by default.

Define:

``` cpp
ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT
```

before including the library to choose another embedded-safe default, or
explicitly configure a receiver maximum of zero when an unbounded queue
is genuinely required.

Queue diagnostics include:

``` cpp
GetPendingEventCount()
GetPeakPendingEventCount()
GetRejectedEventCount()
GetDroppedEventCount()
ResetEventQueueStatistics()
```

Drained Event collections have a separate retained-capacity policy:

``` cpp
EventCollectionCapacityPolicy::Retain
EventCollectionCapacityPolicy::ShrinkWhenUnderutilized
EventCollectionCapacityPolicy::ReleaseAfterDrain
```

For example:

``` cpp
thread.SetEventCollectionCapacityPolicy(
    Event::EventCollectionCapacityPolicy::ShrinkWhenUnderutilized
);

thread.SetMinimumRetainedEventCapacity(4);
thread.SetEventCapacityExcessFactor(2);
```

## `EventThread`

`EventThread` is the principal Event-processing Thread type.

It is built on ESPressio Threads, but differs from a conventional
continuously looping Thread.

An `EventThread` can remain efficiently suspended until an Event
matching one of its registered listeners arrives. The relevant Event
callback is then executed on the Event Thread's own task. Once pending
Events have been processed, the Thread can return to waiting without
consuming CPU cycles merely to poll for work.

Remember:

> An Event can be created and dispatched from anywhere. Only
> Event-capable receiver types need to participate in Event processing.

## `PrecisionEventThread<TTime, Traits>`

`PrecisionEventThread` combines Event processing with the deterministic
iteration model provided by ESPressio Threads:

``` cpp
template<
    typename TTime = Timing::DefaultClockTime,
    typename TRepresentationTraits =
        Threads::PrecisionThreadTraits<TTime>
>
class PrecisionEventThread;
```

Ordinary usage:

``` cpp
class ControlThread :
    public ESPressio::Event::PrecisionEventThread<> {
};
```

Event processing order can be configured using:

``` cpp
PrecisionEventProcessOrder::EventsBeforeIteration
PrecisionEventProcessOrder::EventsAfterIteration
```

Arrival behavior can be configured using:

``` cpp
PrecisionEventArrivalPolicy::ProcessOnNextIteration
PrecisionEventArrivalPolicy::TriggerImmediateIteration
PrecisionEventArrivalPolicy::ProcessImmediately
```

`ProcessImmediately` still means asynchronously as soon as the owning
task is scheduled; Event handlers are not moved onto the dispatching
task.

## `EventListener`

Listeners express interest in a specific Event type and invoke
application code when a matching Event is delivered.

A listener can be registered on an Event-capable receiver:

``` cpp
Event::EventListenerHandlePtr handle =
    eventThread.RegisterListener<
        TemperatureChangeEvent
    >(
        [](TemperatureChangeEvent* event,
           Event::EventDispatchMethod dispatchMethod,
           Event::EventPriority priority) {

            // React to the Event.
        }
    );
```

The returned handle owns the registration lifetime. Destroying or
unregistering the handle removes the listener.

Listeners can therefore be enabled or disabled dynamically without
adding a boolean check to every callback invocation.

### Listener interest

Listener interest policies allow a receiver to reject Events that are
not relevant to it.

`EventListenerInterest::YoungerThan` uses a typed `EventTime` threshold,
converted through Timing traits and compared with the Event's raw
nanosecond age.

Custom interest logic can also be used where application-specific
filtering is required.

## Typed Event Observers

Callback-based listeners can alternatively be represented as typed
Observers.

Implement:

``` cpp
IEventObserver<EventType>
```

and register it with the Event receiver:

``` cpp
class TemperatureObserver :
    public Event::IEventObserver<
        TemperatureChangeEvent
    > {

public:
    void OnEvent(
        TemperatureChangeEvent* event,
        Event::EventDispatchMethod dispatchMethod,
        Event::EventPriority priority
    ) override {
        // React to the Event.
    }
};
```

Registration uses the same asynchronous Event pipeline:

``` cpp
TemperatureObserver observer;

Event::EventListenerHandlePtr observerHandle =
    eventThread.RegisterObserver<
        TemperatureChangeEvent
    >(
        &observer
    );
```

`IEventObserver<EventType>` derives from ESPressio Observable's
`IObserver` contract.

Observers are non-owning: the Observer instance must remain alive until
its registration handle is unregistered or destroyed.

An Observer may implement multiple `IEventObserver<EventType>`
interfaces and register each independently.

## `EventManager`

`EventManager` is the central asynchronous dispatch infrastructure.

It coordinates the transit of Events from dispatching code to the
interested Event receivers without requiring the producer to know those
receivers.

The Event Manager is process-lifetime FreeRTOS infrastructure. Its task,
task-notification wakeup, dispatcher, and per-Event-type routing structures
intentionally remain allocated until device shutdown; this is fixed
infrastructure rather than leaked per-dispatch Event ownership.

------------------------------------------------------------------------

# Type topology

The complete Event type topology is available here:

[![ESPressio Event complete type
topology](diagrams/espressio-event-type-topology.png)](diagrams/espressio-event-type-topology.png)

The editable vector source is available at:

[`diagrams/espressio-event-type-topology.svg`](diagrams/espressio-event-type-topology.svg)

------------------------------------------------------------------------

# Usage example: a decoupled thermometer

The following example illustrates the core purpose of the library.

We want three independent pieces of functionality:

``` text
Thermometer
    -> reads a sensor

TemperatureSerialLogger
    -> reports changes to Serial

TemperatureDisplay
    -> updates a physical display
```

None should need a direct reference to either of the others.

Their only shared contract is:

``` cpp
TemperatureChangeEvent
```

## `TemperatureChangeEvent`

``` cpp
#pragma once

#include <ESPressio_Event.hpp>

class TemperatureChangeEvent final :
    public ESPressio::Event::Event<> {

private:
    const int _previousTemperature;
    const int _newTemperature;

public:
    TemperatureChangeEvent(
        int previousTemperature,
        int newTemperature
    ) :
        _previousTemperature(previousTemperature),
        _newTemperature(newTemperature) {
    }

    int GetPreviousTemperature() const {
        return _previousTemperature;
    }

    int GetNewTemperature() const {
        return _newTemperature;
    }
};
```

Both values are supplied through the constructor and no setters are
exposed. The Event therefore describes one complete temperature
transition.

## `TemperatureSerialLogger`

``` cpp
#pragma once

#include <Arduino.h>
#include <ESPressio_EventThread.hpp>

#include "TemperatureChangeEvent.hpp"

class TemperatureSerialLogger final :
    public ESPressio::Event::EventThread {

private:
    ESPressio::Event::EventListenerHandlePtr
        _temperatureChangeListener =
            RegisterListener<
                TemperatureChangeEvent
            >(
                [](TemperatureChangeEvent* event,
                   ESPressio::Event::EventDispatchMethod,
                   ESPressio::Event::EventPriority) {

                    const int change =
                        event->GetNewTemperature() -
                        event->GetPreviousTemperature();

                    const char* direction =
                        change >= 0 ? "UP" : "DOWN";

                    const int magnitude =
                        change >= 0 ? change : -change;

                    Serial.printf(
                        "Temperature is %s by %d degrees "
                        "(from %d to %d).\n",
                        direction,
                        magnitude,
                        event->GetPreviousTemperature(),
                        event->GetNewTemperature()
                    );
                }
            );

public:
    TemperatureSerialLogger() :
        EventThread(false) {
    }
};
```

The logger contains no sensor code and no display code. It understands
only the Event contract.

## `TemperatureDisplay`

A display implementation can independently register for exactly the same
Event:

``` cpp
#pragma once

#include <ESPressio_EventThread.hpp>

#include "TemperatureChangeEvent.hpp"

class TemperatureDisplay final :
    public ESPressio::Event::EventThread {

private:
    ESPressio::Event::EventListenerHandlePtr
        _temperatureChangeListener =
            RegisterListener<
                TemperatureChangeEvent
            >(
                [this](TemperatureChangeEvent* event,
                       ESPressio::Event::EventDispatchMethod,
                       ESPressio::Event::EventPriority) {

                    DisplayTemperature(
                        event->GetNewTemperature()
                    );
                }
            );

    void DisplayTemperature(int temperature) {
        // Update the application's physical display.
    }

public:
    TemperatureDisplay() :
        EventThread(false) {
    }
};
```

Again, there is no relationship to the logger or sensor implementation.

## `Thermometer`

The sensor-side code only needs to dispatch the Event:

``` cpp
#pragma once

#include "TemperatureChangeEvent.hpp"

class Thermometer {

private:
    int _temperature = 0;

    int ReadTemperatureSensor() {
        // Replace with the appropriate sensor implementation.
        return _temperature;
    }

public:
    void UpdateTemperature() {
        const int temperature =
            ReadTemperatureSensor();

        if (temperature == _temperature) {
            return;
        }

        const int previousTemperature =
            _temperature;

        _temperature = temperature;

        (new TemperatureChangeEvent(
            previousTemperature,
            temperature
        ))->Queue();
    }
};
```

The significant line is simply:

``` cpp
(new TemperatureChangeEvent(
    previousTemperature,
    temperature
))->Queue();
```

The Thermometer does not know which modules---if any---will process the
Event.

## Example topology

[![Thermometer example decoupled event
topology](diagrams/thermometer-example-topology.png)](diagrams/thermometer-example-topology.png)

The editable vector source is available at:

[`diagrams/thermometer-example-topology.svg`](diagrams/thermometer-example-topology.svg)

The important result is:

``` text
TemperatureSerialLogger
    has no direct relationship with
    TemperatureDisplay or Thermometer

TemperatureDisplay
    has no direct relationship with
    TemperatureSerialLogger or Thermometer

Thermometer
    has no direct relationship with
    TemperatureSerialLogger or TemperatureDisplay
```

Yet both consumers react independently whenever `Thermometer` dispatches
a `TemperatureChangeEvent`.

Additional consumers can be introduced later without modifying the
existing producer or consumers.

That is the central architectural advantage of ESPressio Event.

------------------------------------------------------------------------

# Precision Event Thread example

``` cpp
#include <ESPressio_Event.hpp>
#include <ESPressio_PrecisionEventThread.hpp>

using namespace ESPressio;

class SetpointEvent final :
    public Event::Event<> {

private:
    const int _setpoint;

public:
    explicit SetpointEvent(int setpoint) :
        _setpoint(setpoint) {
    }

    int GetSetpoint() const {
        return _setpoint;
    }
};

class ControlThread final :
    public Event::PrecisionEventThread<> {

private:
    int _setpoint = 0;

protected:
    void OnIteration(
        IterationTime delta,
        IterationTime startTime,
        Threads::SkippedIterationCount skippedIterations
    ) override {
        (void)delta;
        (void)startTime;
        (void)skippedIterations;

        // Perform deterministic control work.
    }

public:
    void ApplySetpoint(SetpointEvent* event) {
        _setpoint = event->GetSetpoint();
    }
};
```

See:

``` text
examples/PrecisionEventThread
```

for the complete repository example.

------------------------------------------------------------------------

# Optional Serializable Events

Serializable Event support is opt-in:

``` cpp
#include <ESPressio_Event_Serializable.hpp>
```

or:

``` cpp
#include <ESPressio_SerializableEvent.hpp>
```

Neither is imported by the normal `ESPressio_Event.hpp` path.

A consuming application using Serializable Events declares ESPressio
Serializable explicitly:

``` ini
lib_deps =
    flowduino/ESPressio-Event@^5.8.4
    flowduino/ESPressio-Serializable@^0.10.2
```

## `SerializableEvent`

The optional base is:

``` cpp
template<
    typename TDerived,
    typename TTime =
        Units::SerializableNanoSeconds<uint64_t>
>
class SerializableEvent;
```

Example:

``` cpp
#include <ESPressio_Event_Serializable.hpp>

class TemperatureEvent final :
    public ESPressio::Event::SerializableEvent<
        TemperatureEvent
    > {

private:
    float _temperature = 0.0f;
    uint32_t _sensorId = 0;

public:
    ESPRESSIO_SERIALIZABLE_TYPE(
        TemperatureEvent
    )

    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY(
            "temperature",
            _temperature
        ),
        ESPRESSIO_PROPERTY(
            "sensorId",
            _sensorId
        )
    )
};
```

The derived payload can use the ESPressio Serializable feature set,
including JSON, CBOR, Binary, schema versions, aliases, validation,
migrations, streaming, and schema introspection.

## What is not serialized

Local Event runtime state is deliberately not Event payload:

``` text
reference count
local dispatch status
local System Clock dispatch timestamp
routing/listener state
```

A deserialized Serializable Event is a **new local Event** which can
subsequently be queued or stacked normally.

Transport metadata belongs either explicitly in the Event payload or in
the transport envelope.

## Ordinary Events remain serialization-free

This:

``` cpp
#include <ESPressio_Event.hpp>

class ButtonEvent :
    public ESPressio::Event::Event<> {
};
```

does not require ESPressio Serializable.

------------------------------------------------------------------------

# Observer-to-Event bridges

ESPressio Event can optionally convert synchronous Observer
notifications from other ESPressio libraries into asynchronous Events.

For genuinely upstream libraries, the dependency direction is:

``` text
Timing / Threads / Security / Command
    expose synchronous Observer APIs

Event
    optionally consumes those APIs
    and emits asynchronous Events
```

The originating libraries therefore do not need to depend upward on Event.

Sockets and ESP-Now are special cases in 5.8.x because each already optionally consumes Event to implement concrete Event transports. Their existing bridges remain available for compatibility, but should move downstream in a future integration relocation.

## System Clock Event Bridge

Timing 2.2 System Clock Observer notifications can be bridged using:

``` cpp
#include <ESPressio_SystemClockEventBridge.hpp>

Event::SystemClockEventBridge::
    GetInstance().
    Initialize();
```

Timing Events are grouped under:

``` text
src/timing-events/
```

with:

``` cpp
#include <ESPressio_TimingEvents.hpp>
```

Every `ISystemClockObserver` callback has a corresponding strongly typed
Event carrying the relevant callback snapshot, including synchronization
before/after values, clock difference, result/status, state changes,
configuration changes, and callback lifecycle information.

Serializable counterparts are separately opt-in:

``` cpp
#include <ESPressio_TimingEvents_Serializable.hpp>
#include <ESPressio_SystemClockEventBridge_Serializable.hpp>

Event::SerializableSystemClockEventBridge::
    GetInstance().
    Initialize();
```

Both bridges remain dormant until explicitly initialized.

## Threads infrastructure Event Bridges

The singleton infrastructure Observer APIs in ESPressio Threads can
similarly be bridged through:

``` cpp
ThreadManagerEventBridge
ThreadGarbageCollectorEventBridge
ThreadTerminationDispatcherEventBridge
```

and the opt-in Serializable counterparts:

``` cpp
SerializableThreadManagerEventBridge
SerializableThreadGarbageCollectorEventBridge
SerializableThreadTerminationDispatcherEventBridge
```

Thread Events are grouped beneath:

``` text
src/thread-events/
```

with:

``` cpp
#include <ESPressio_ThreadEvents.hpp>
#include <ESPressio_ThreadEvents_Serializable.hpp>
```

Bridge batch headers are:

``` cpp
#include <ESPressio_ThreadEventBridges.hpp>
#include <ESPressio_ThreadEventBridges_Serializable.hpp>
```

The ordinary bridges do not require ESPressio Serializable.

## Security Event Bridge

`TransportSecurityEventBridge` observes one selected `Security::TransportSecurity` instance and emits asynchronous Events for configuration changes, session establishment/reset, replay-protection resets, and security failures.

``` cpp
#include <ESPressio_TransportSecurityEventBridge.hpp>

ESPressio::Event::TransportSecurityEventBridge bridge;
bridge.Initialize(security);
```

The bridge is instance-oriented because `TransportSecurity` is application-owned state rather than a process singleton.

## Command Registry Event Bridge

`CommandRegistryEventBridge` converts command registration/unregistration lifecycle notifications from the process-wide `CommandRegistry` into Events. It deliberately does not convert normal Command execution callbacks or middleware into a competing Event execution mechanism.

## Socket Event Bridges

For 5.8.x compatibility, socket lifecycle bridges remain split according to dependency ownership:

``` text
SocketWorkerEventBridge
    -> worker start / start failure / stop
    -> requires Sockets only

SocketSecuritySessionEventBridge
    -> secure-session fault / reset
    -> requires Sockets + Security
```

Because Sockets also implements concrete Event transports, these bridge implementations should ultimately move downstream into Sockets' optional Event integration to eliminate the reciprocal optional dependency edge.

## ESP-Now Transport Event Bridge

`ESPNowTransportEventBridge` converts the ESP-NOW transport's initialization, shutdown, peer lifecycle, and send-result observer notifications into asynchronous Events while leaving protocol receive delivery with the existing ESP-NOW handler mechanism.

Event 5.8.4 validates this bridge against ESPressio ESP-Now 0.5.3. Because ESP-Now also implements `ESPNowEventTransport`, this bridge should ultimately move downstream into ESP-Now's optional Event integration to eliminate the reciprocal optional dependency edge.

All 5.8 bridge families remain dormant until explicitly initialized.

------------------------------------------------------------------------

# Event Transport

Version 5.3 introduced a transport-neutral bidirectional routing layer
for Serializable Events, extended in 5.4 with per-transport routing
policy.

The subsystem is opt-in:

``` cpp
#include <ESPressio_EventTransport.hpp>
```

Concrete transport implementations live outside ESPressio Event and
implement:

``` cpp
IEventTransport
```

ESPressio Event owns:

``` text
Event type registration
Binary serialization/deserialization
stable wire identities
inbound/outbound policy
dispatch metadata
pending-work lifecycle
remote-to-local loop prevention
per-transport routing policy
```

## Stable Event type identity

Every transported Event declares a stable wire identity:

``` cpp
ESPRESSIO_EVENT_TRANSPORT_TYPE(
    MySerializableEvent,
    "com.example.my-event.v1"
)
```

The stable name is converted to the wire identifier; RTTI implementation
names are not used as protocol contracts.

## Registering Event directions

Global/default policy:

``` cpp
manager.RegisterInboundEvent<RemoteCommandEvent>();
manager.RegisterOutboundEvent<TelemetryEvent>();
manager.RegisterBidirectionalEvent<SharedStateEvent>();
```

C++17 bulk forms are available:

``` cpp
manager.RegisterOutboundEvents<
    TelemetryEvent,
    DiagnosticsEvent,
    BatteryStatusEvent
>();
```

A concrete transport can have its own override:

``` cpp
manager.RegisterOutboundEvent<
    TelemetryEvent
>(
    &udpTransport
);

manager.RegisterBidirectionalEvent<
    SharedStateEvent
>(
    &espNowTransport
);
```

The model is:

``` text
global/default direction
        +
optional per-transport override
```

A transport-specific override is authoritative for that transport,
including `EventTransportDirection::None`.

## Multiple transports

Multiple transports can be registered simultaneously:

``` cpp
manager.RegisterTransport(
    &espNowTransport
);

manager.RegisterTransport(
    &udpTransport
);
```

The same Event can therefore have different routing policy on different
physical/network mechanisms.

## Unregistration and pending work

Unregistration mirrors registration and includes bulk forms.

Pending work can be independently configured to complete or be discarded
using:

``` cpp
EventTransportUnregistrationOptions
```

Transport-scoped unregistration affects only matching work for the
selected transport.

## Remote origin and loop prevention

`EventDispatchContext` records whether an Event originated locally or
remotely, together with transport metadata such as message ID and hop
count.

Remotely received Events are dispatched normally to local listeners but
are **not retransmitted by default**, preventing simple transport loops
such as:

``` text
A -> B -> A -> B ...
```

## Wire envelope

The version-1 transport envelope preserves:

``` text
stable Event type ID
Serializable schema version
message ID
dispatch method
priority
hop count
payload length
```

The payload uses the ESPressio Serializable **ESPB v2 binary wire format**.
Event 5.8.4 validates against Serializable 0.10.2 while preserving the existing wire representation and direct binary transport path, with the archive path retained for compatibility/schema-migration cases.

## EventTransportManager observation

`EventTransportManager` exposes an `IEventTransportManagerObserver`
surface for transport registration, type/route lifecycle, and
inbound/outbound diagnostics.

------------------------------------------------------------------------

## Event Transport Transaction Observation

Version 5.5 adds a unified, transport-neutral observation surface for complete Event Transport transaction activity.

Existing `IEventTransportManagerObserver` callbacks remain available and unchanged. The interface additionally exposes:

```cpp
virtual void OnEventTransportTransaction(
    const EventTransportTransaction& transaction
) {}
```

`EventTransportTransaction` is an immutable callback snapshot describing the transaction stage and the context that is available at that stage. It can include:

```text
transaction stage
inbound / outbound direction
stable Event type ID
stable Event type name
Serializable schema version
transport message ID
concrete IEventTransport pointer
borrowed IEvent pointer when a local/reconstructed Event exists
borrowed serialized Binary payload when available
dispatch method
Event priority
local / remote origin
hop count
transport handoff acceptance result
```

Transaction stages include:

```cpp
EventTransportTransactionStage::OutboundAccepted
EventTransportTransactionStage::OutboundSerialized
EventTransportTransactionStage::OutboundHandedToTransport
EventTransportTransactionStage::InboundAccepted
EventTransportTransactionStage::InboundRejected
EventTransportTransactionStage::InboundDeserialized
EventTransportTransactionStage::InboundDispatched
EventTransportTransactionStage::Failed
```

The transaction callback is intended for diagnostics, tracing, monitoring, telemetry, persistence, and similar cross-cutting integrations without coupling ESPressio Event to any particular output mechanism.

For example, ESPressio Serial Event Monitor can subscribe once to `EventTransportManager` and observe every Serializable Event Transport transaction without implementing or wrapping a physical Event transport.

### Borrowed lifetime

`EventTransportTransaction::Event` and `EventTransportTransaction::Payload` are **borrowed callback-lifetime references**.

They are valid only for the duration of `OnEventTransportTransaction(...)`. An Observer that needs to retain either representation must copy the required data before returning.

This preserves the existing Event ownership/ref-count contract and avoids diagnostic observers extending Event or transport-buffer lifetime.

### Stable human-readable type identity

The runtime transport registration now retains the stable transport type name declared through:

```cpp
ESPRESSIO_EVENT_TRANSPORT_TYPE(
    MyEvent,
    "com.example.my-event.v1"
)
```

Transaction Observers therefore receive both the hashed wire `EventTypeID` and the stable human-readable `EventTypeName`.

### Representation neutrality

Event Transport continues to use ESPressio Serializable Binary payloads on the wire. Transaction observation exposes that already-produced payload where available but does not introduce JSON or another diagnostic representation into Event itself.

A diagnostics/Serial library can decide how to present the transaction—summary, hexadecimal payload, or another representation—without forcing that cost or dependency onto every Event Transport application.

# Migration from 4.x

## Event inheritance

4.x:

``` cpp
class MyEvent :
    public Event::Event {
};
```

5.x:

``` cpp
class MyEvent :
    public Event::Event<> {
};
```

## PrecisionEventThread inheritance

4.x:

``` cpp
class MyThread :
    public Event::PrecisionEventThread {
};
```

5.x:

``` cpp
class MyThread :
    public Event::PrecisionEventThread<> {
};
```

## Event time

Timing 1.x's fixed `ClockTime` is no longer the Event representation
contract.

The default representation is:

``` cpp
Timing::DefaultClockTime
```

Generic code should prefer:

``` cpp
typename MyEvent::TimeType
```

## Type-erased lifecycle timing

`IEvent` infrastructure uses:

``` cpp
GetDispatchTimeNanoseconds()
GetTimeSinceDispatchNanoseconds()
```

while concrete typed Events expose:

``` cpp
event->GetDispatchTime()
event->GetTimeSinceDispatch()
```

------------------------------------------------------------------------

# Design summary

The core 5.x architecture is:

``` text
                         IEvent
                  type-erased Event core
                         |
                         v
                    Event<TTime>
                         |
              +----------+----------+
              |                     |
              v                     v
     DefaultClockTime      SerializableNanoSeconds
                                    |
                                    v
                         SerializableEvent<TDerived>
```

and:

``` text
Threads::PrecisionThread<TTime, Traits>
                    |
                    v
PrecisionEventThread<TTime, Traits>
```

The wider integration architecture is:

``` text
                 ESPressio Event
                       |
        +--------------+--------------+
        |              |              |
        v              v              v
 local routing    Event Bridges   Event Transport
        |              |              |
        |    Timing / Threads /        |
        |    Security / Command /      |
        |    Sockets / ESP-Now         |
        |       Observers              |
        |                             |
        +-----------------------------+
                       |
                Event contracts
```

The library therefore provides one Event engine and one asynchronous
routing system while allowing:

-   the public time representation to vary at compile time;
-   synchronous subsystem notifications to be bridged into Events only
    when requested;
-   Serializable Events to remain optional;
-   concrete network/radio transports to remain outside Event;
-   global and per-transport routing policy to coexist.

Most importantly, the original Event-driven design principle remains
unchanged:

> **Application modules communicate through Event contracts rather than
> acquiring direct relationships with one another.**

That is the purpose of ESPressio Event.

## ESPressio Threads 3.1 compatibility

Version 5.6.1 defines explicit equality semantics for `EventDispatchContext`, allowing the context to participate correctly in ESPressio Threads 3.1 `ReadWriteMutex<T>` change detection.

Two contexts are equal only when all dispatch identity fields match:

```text
Origin
TransportMessageID
HopCount
```

This is a compatibility fix only; Event dispatch behaviour and public Event Transport semantics are unchanged.