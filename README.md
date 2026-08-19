# ESPressio Event

Event-Driven Observer Pattern Components of the Flowduino ESPressio Development Platform.

ESPressio Event provides asynchronous typed Event routing, Event-aware Threads, bounded receiver queues, listener/observer registration, priority dispatch, and high-resolution Event timing for ESP32 applications.

## Version 5.2.0

Version `5.1.0` extends the 5.x architecture with opt-in System Clock Observer-to-Event bridging, aligned with:

```text
ESPressio Threads >= 3.0.0
ESPressio Observable >= 3.0.0
ESPressio Timing >= 2.2.0
```

Timing 2.2 is now also declared directly because the optional System Clock Event bridge compiles against its public Observer API. ESPressio Units remains available transitively through the dependency stack.

Serializable Event support remains optional: ordinary Event and Timing Event bridge users do not acquire an ESPressio Serializable dependency.




### 5.2.0 ESPressio Threads infrastructure Event bridges

Version `5.2.0` adds opt-in bridges from the singleton infrastructure Observer APIs introduced by ESPressio Threads `3.1.0` into asynchronous ESPressio Events.

Three ordinary singleton bridges are provided:

```cpp
ThreadManagerEventBridge
ThreadGarbageCollectorEventBridge
ThreadTerminationDispatcherEventBridge
```

and three opt-in Serializable counterparts:

```cpp
SerializableThreadManagerEventBridge
SerializableThreadGarbageCollectorEventBridge
SerializableThreadTerminationDispatcherEventBridge
```

Thread-specific Events are grouped beneath:

```text
src/thread-events/
```

with batch headers:

```cpp
#include <ESPressio_ThreadEvents.hpp>
#include <ESPressio_ThreadEvents_Serializable.hpp>
```

Bridge batch headers are also available:

```cpp
#include <ESPressio_ThreadEventBridges.hpp>
#include <ESPressio_ThreadEventBridges_Serializable.hpp>
```

All bridges remain dormant until explicitly initialized:

```cpp
Event::ThreadManagerEventBridge::GetInstance().Initialize();
Event::ThreadGarbageCollectorEventBridge::GetInstance().Initialize();
Event::ThreadTerminationDispatcherEventBridge::GetInstance().Initialize();
```

Each bridge converts the corresponding synchronous Threads Observer callback into a queued asynchronous Event.

The Event payloads use immutable Threads snapshots/results. Raw `IThread*` values are deliberately not retained as asynchronous object references. The registration-failure Event records only the numeric address for local diagnostic correlation.

Serializable counterparts flatten Thread state, cleanup results, garbage-collection results, and execution modes into stable primitive schema fields. `std::exception_ptr` values are represented by portable exception-message strings.

ESPressio Serializable remains an optional dependency. Ordinary Thread Events and ordinary Thread Event Bridges do not include or require it.

### 5.1.0 Timing Event bridge

Version `5.1.0` adds an explicit bridge from ESPressio Timing 2.2 System Clock Observer notifications into asynchronous ESPressio Events.

The ordinary bridge is opt-in:

```cpp
#include <ESPressio_SystemClockEventBridge.hpp>

Event::SystemClockEventBridge::GetInstance().Initialize();
```

Timing Events are grouped under `src/timing-events/` and batch-imported by:

```cpp
#include <ESPressio_TimingEvents.hpp>
```

Every `ISystemClockObserver` callback has a corresponding strongly typed Event carrying the callback snapshot, including synchronization before/after values, immediate clock difference, synchronization result/status, state changes, configuration changes, and callback lifecycle information.

Serializable counterparts are also provided without making ESPressio Serializable a mandatory dependency:

```cpp
#include <ESPressio_TimingEvents_Serializable.hpp>
#include <ESPressio_SystemClockEventBridge_Serializable.hpp>

Event::SerializableSystemClockEventBridge::GetInstance().Initialize();
```

Serializable timing Events flatten Timing synchronization/configuration structures into stable primitive payload fields. `std::exception_ptr` is deliberately converted to a portable exception-message string in the Serializable callback-failure Event.

Both bridges are dormant until `Initialize()` is explicitly called, and `Shutdown()` releases the retained Observable registration handle.

### 5.0.1 corrective fixes

This patch release resolves several defects discovered after the initial 5.0.0 release:

- fixes `ESPressio_EventListener.hpp` compilation by explicitly including the Observable exception declaration and its direct standard-library dependencies;
- neutralizes the obsolete `ESPressio_EventListener.cpp` template definitions which no longer matched the header API;
- removes the obsolete Arduino `String GetThreadNamePrefix()` API from `EventThread`;
- removes public-header namespace pollution from the corrected Event Thread and enum headers;
- fixes `EventListenerInterest` increment/decrement wrap-around to use its actual three enum values;
- hardens Event reference counting against unmatched `__unref()` underflow;
- prevents EventDispatcher from creating empty receiver buckets for unhandled Event types;
- removes raw heap allocation for dispatcher receiver buckets;
- avoids invoking receiver code while holding the EventDispatcher receiver-map mutex;
- removes empty receiver buckets when the last receiver unregisters.

The Event 5.x generic Timing/Threads architecture and optional Serializable Event design remain unchanged.

## Compatibility

ESPressio Event targets the ESP32 family using Arduino-ESP32.

The implementation uses ESP-IDF FreeRTOS facilities, C++ RTTI, `std::shared_mutex`, and Arduino APIs through its dependency stack.

RTTI must be enabled:

```ini
build_unflags =
    -fno-rtti
```

## Dependencies

Normal Event applications require only the dependencies declared by the library:

```ini
lib_deps =
    flowduino/ESPressio-Event@^5.2.0
```

The mandatory dependency graph is:

```text
ESPressio Event 5.x
    |
    +-- ESPressio Threads >= 3.0.0
    |       |
    |       +-- ESPressio Timing 2.x
    |       +-- ESPressio Units
    |
    +-- ESPressio Observable >= 3.0.0
```

**ESPressio Serializable is not a mandatory Event dependency.**

Only applications that explicitly include the optional Serializable Event header need to add ESPressio Serializable.

## Generic Event Time Representation

The base Event is now:

```cpp
template<
    typename TTime = Timing::DefaultClockTime
>
class Event;
```

Ordinary Event code therefore uses:

```cpp
class MyEvent :
    public ESPressio::Event::Event<> {
};
```

A different Timing-compatible public representation can be selected:

```cpp
using MyTime = SomeCompatibleTimeType;

class MyEvent :
    public ESPressio::Event::Event<MyTime> {
};
```

### Type-erased Event engine

`IEvent` remains non-templated.

Routing, receivers, the Event Manager, reference ownership, and listeners continue to operate using:

```cpp
IEvent*
```

The Event engine exposes raw lifecycle timing through:

```cpp
uint64_t GetDispatchTimeNanoseconds() const;
uint64_t GetTimeSinceDispatchNanoseconds() const;
```

while `Event<TTime>` exposes the strongly typed API:

```cpp
TTime GetDispatchTime() const;
TTime GetTimeSinceDispatch() const;
```

This keeps the routing infrastructure independent of the selected public Unit representation.

## Internal Timing State

Event lifecycle timing is stored internally as raw nanoseconds:

```text
dispatch state
    |
    +-- was dispatched
    +-- dispatch time nanoseconds
```

The selected `TTime` representation is created only at the public API boundary through:

```cpp
Timing::TimeTraits<TTime>
```

This means choosing a Serializable or another richer Unit representation does not add representation-specific state to Event routing.

The Event System Clock uses:

```cpp
Timing::SystemClock<TTime>::GetInstance()
```

Timing 2.x typed System Clock facades share one underlying global System Clock core.

## Listener Age Filtering

`EventListenerInterest::YoungerThan` is now representation-independent.

The public threshold remains the default `EventTime` Unit, but the listener converts it through:

```cpp
Timing::TimeTraits<EventTime>
```

and compares it directly with the Event's type-erased raw nanosecond age.

This removes the former dependency on Timing 1.x `ClockBase` conversion helpers.

## PrecisionEventThread

`PrecisionEventThread` now mirrors the generic ESPressio Threads 3.x Precision Thread model:

```cpp
template<
    typename TTime = Timing::DefaultClockTime,
    typename TRepresentationTraits =
        Threads::PrecisionThreadTraits<TTime>
>
class PrecisionEventThread;
```

Ordinary usage:

```cpp
class ControlThread :
    public ESPressio::Event::PrecisionEventThread<> {
};
```

A different time representation can be selected without duplicating the Event-processing or precision-scheduling implementation.

The injected clock type is:

```cpp
Timing::ISystemClock<
    PrecisionEventThread::IterationTime
>*
```

and the default clock uses the shared Timing 2.x System Clock.

### Event processing policies

The existing policies remain:

```cpp
PrecisionEventProcessOrder::EventsBeforeIteration
PrecisionEventProcessOrder::EventsAfterIteration
```

and:

```cpp
PrecisionEventArrivalPolicy::ProcessOnNextIteration
PrecisionEventArrivalPolicy::TriggerImmediateIteration
PrecisionEventArrivalPolicy::ProcessImmediately
```

## Optional Serializable Events

Serializable Event support lives in:

```cpp
#include <ESPressio_Event_Serializable.hpp>
```

or the equivalent convenience umbrella:

```cpp
#include <ESPressio_SerializableEvent.hpp>
```

Neither header is imported by normal `ESPressio_Event.hpp`.

A consuming application which uses Serializable Events declares:

```ini
lib_deps =
    flowduino/ESPressio-Event@^5.2.0
    flowduino/ESPressio-Serializable@^0.9.0
```

### SerializableEvent

The optional base is:

```cpp
template<
    typename TDerived,
    typename TTime =
        Units::SerializableNanoSeconds<uint64_t>
>
class SerializableEvent;
```

Example:

```cpp
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

    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(
        1
    )

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

The inherited Event `TimeType` is itself a Serializable ESPressio Unit.

The derived Event payload can therefore use the complete ESPressio Serializable feature set, including JSON, CBOR, Binary, schema versions, aliases, validation, migrations, streaming, and schema introspection.

## What Is Not Serialized

The Event engine deliberately does **not** serialize local runtime lifecycle state.

The following are not Event payload:

```text
reference count
local dispatch status
local System Clock dispatch timestamp
routing/listener state
```

Serializing these values would produce incorrect semantics when an Event is transmitted to another ESP32 or restored after restart.

A deserialized Serializable Event is therefore a **new local Event** which can subsequently be queued or stacked normally.

Transport metadata such as origin device identity, correlation IDs, sequence numbers, or source timestamps should be represented explicitly in the Event payload or in a separate transport envelope.

## Ordinary Events Remain Serialization-Free

This code:

```cpp
#include <ESPressio_Event.hpp>

class ButtonEvent :
    public ESPressio::Event::Event<> {
};
```

does not include, compile, link, or require ESPressio Serializable.

The normal Event library metadata deliberately contains no Serializable dependency.

This preserves the pay-for-what-you-use design:

```text
ordinary Event
    -> no Serializable dependency

SerializableEvent
    -> consuming application opts into Serializable
```

## Migration from 4.x

### Event

4.x:

```cpp
class MyEvent :
    public Event::Event {
};
```

5.x:

```cpp
class MyEvent :
    public Event::Event<> {
};
```

### PrecisionEventThread

4.x:

```cpp
class MyThread :
    public Event::PrecisionEventThread {
};
```

5.x:

```cpp
class MyThread :
    public Event::PrecisionEventThread<> {
};
```

### EventTime

Timing 1.x's global:

```cpp
Timing::ClockTime
```

is no longer used.

The default Event representation is:

```cpp
Timing::DefaultClockTime
```

and generic Event code should prefer:

```cpp
typename MyEvent::TimeType
```

### IEvent lifecycle timing

`IEvent` no longer exposes a fixed typed return value for dispatch time.

Type-erased infrastructure should use:

```cpp
GetDispatchTimeNanoseconds()
GetTimeSinceDispatchNanoseconds()
```

Concrete Event consumers use:

```cpp
event->GetDispatchTime()
event->GetTimeSinceDispatch()
```

on their typed `Event<TTime>` descendant.

## Example: Precision Event Thread

```cpp
class SetpointEvent final :
    public Event::Event<> {
    // payload
};

class ControlThread final :
    public Event::PrecisionEventThread<> {

protected:
    void OnIteration(
        IterationTime delta,
        IterationTime startTime,
        Threads::SkippedIterationCount skipped
    ) override {
        // ...
    }
};
```

See:

```text
examples/PrecisionEventThread
```

## Example: Serializable Event

See:

```text
examples/SerializableEvent
```

The example defines a Serializable Event payload and serializes it through the normal ESPressio Serializable archive API.

## Design Summary

Event 5.0 establishes these boundaries:

```text
                         IEvent
                  type-erased Event core
                         |
                         v
                    Event<TTime>
                         |
              +----------+----------+
              |                     |
              v                     v
 DefaultClockTime        SerializableNanoSeconds
                                    |
                                    v
                         SerializableEvent<TDerived>
```

and:

```text
Threads::PrecisionThread<TTime, Traits>
                    |
                    v
PrecisionEventThread<TTime, Traits>
```

The architecture provides one Event engine, one routing system, and one PrecisionEventThread implementation while allowing the public Unit representation to vary at compile time.

Most importantly, optional serialization remains genuinely optional.
