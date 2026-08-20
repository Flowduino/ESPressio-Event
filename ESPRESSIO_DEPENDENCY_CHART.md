# ESPressio Dependency Chart

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.png)

## Purpose

This document describes the dependency relationships between the current ESPressio libraries and the validated release baselines used by the 2026-08-20 dependency-refresh generation.

The chart is hierarchical: libraries with no **required** ESPressio dependencies are positioned at the top, while libraries that build on progressively more of the ESPressio ecosystem appear lower in the hierarchy.

### Relationship notation

- **Solid arrow** — a required ESPressio library dependency.
- **Dashed arrow** — an opt-in relationship. The dependency is required only when the consuming application selects the associated feature, integration, type, or header.
- Arrows point from the **dependent library** to the **library it consumes**.

Optional integrations are intentionally kept out of the normal/core include path wherever possible so that a project does not acquire unrelated ESPressio dependencies simply by consuming a library.

---

## ESPressio Units 0.2.1

**Required ESPressio dependencies: none.**

ESPressio Units is a foundational library providing strongly typed physical quantities and unit representations.

### Units → Serializable — opt-in

Ordinary Unit headers do not require ESPressio Serializable. Serializable counterparts are supplied through separate `*_Serializable.hpp` headers and the `ESPressio_SerializableUnits.hpp` batch header.

```text
ordinary Unit type
    -> ESPressio Units only

Serializable Unit type
    -> ESPressio Units
    -> ESPressio Serializable
```

---

## ESPressio Observable 3.0.1

**Required ESPressio dependencies: none.**

Observable is the foundational synchronous observation/notification mechanism. Version 3.0.1 adds the zero-Observer fast path used by higher-frequency consumers without changing the public Observer contract.

It is directly consumed by:

- ESPressio Timing;
- ESPressio Threads;
- ESPressio Event.

---

## ESPressio Serializable 0.10.0

**Required ESPressio dependencies: none.**

Serializable is the foundational serialization framework. Version 0.10.0 adds direct ESPB v2 binary serialization/deserialization while preserving the existing archive/tree APIs and wire representation.

Current opt-in relationships include:

- ESPressio Units → Serializable for Serializable Unit variants;
- ESPressio Event → Serializable for Serializable Events, runtime Serializable Event construction, and Event Transport;
- ESPressio Serial → Serializable for EventConsole/EventMonitor integrations that consume Serializable payload facilities.

---

## ESPressio Timing 2.2.2

**Required ESPressio dependencies:**

- ESPressio Units `>=0.2.1 <1.0.0`;
- ESPressio Observable `>=3.0.1 <4.0.0`.

### Timing → Units — required

Timing uses ESPressio Units for its strongly typed time representations.

### Timing → Observable — required

Timing exposes logical clock and synchronization lifecycle notifications through ESPressio Observable. The 2.2.2 patch explicitly raises the Observable floor to 3.0.1.

Timing itself does not directly require Serializable. Serializable Unit time representations remain selected by the consuming application through the Units integration.

---

## ESPressio Threads 3.1.2

**Required ESPressio dependencies:**

- ESPressio Timing `>=2.2.2 <3.0.0`;
- ESPressio Observable `>=3.0.1 <4.0.0`.

### Threads → Timing — required

Threads uses Timing for PrecisionThread time-related execution and scheduling infrastructure.

### Threads → Observable — required

Threads uses Observable for synchronous Thread and singleton-infrastructure lifecycle notifications.

The 3.1.2 patch raises both dependency floors to the refreshed upstream patch generation.

---

## ESPressio Event 5.7.1

**Required ESPressio dependencies:**

- ESPressio Threads `>=3.1.2 <4.0.0`;
- ESPressio Timing `>=2.2.2 <3.0.0`;
- ESPressio Observable `>=3.0.1 <4.0.0`.

### Event → Threads — required

Event uses ESPressio Threads for asynchronous Event processing and execution infrastructure.

### Event → Timing — required

Event uses Timing for Event lifecycle timing and the optional System Clock Event Bridge.

### Event → Observable — required

Observable is used directly by Event and by the Observer/Event bridge architecture.

### Event → Serializable — opt-in

Ordinary local Event usage does not require Serializable.

Serializable Event support, runtime Serializable Event construction and Event Transport use ESPressio Serializable 0.10.0 or newer within the current 0.x integration line. Event 5.7 uses the 0.10.0 direct ESPB v2 binary path for normal same-schema transport while retaining the archive path for migration/compatibility cases.

```text
ordinary local Event
    -> no Serializable requirement

Serializable / remotely transported Event
    -> ESPressio Serializable
```

---

## ESPressio ESP-Now 0.2.3

**Required ESPressio dependencies:**

- ESPressio Timing `>=2.2.2 <3.0.0`.

### ESP-Now → Timing — required

ESP-Now provides ESP-NOW-based distributed System Clock synchronization and submits transport-specific observations to ESPressio Timing.

### ESP-Now → Event — opt-in

`ESPNowEventTransport` implements the ESPressio Event transport abstraction. The validated 0.2.3 integration baseline is ESPressio Event 5.7.1 within the 5.x line.

Applications using ESP-Now only for communication and/or System Clock synchronization do not acquire Event or Serializable dependencies.

---

## ESPressio Sockets 0.2.3

**Required ESPressio dependencies: none.**

The core Sockets library deliberately has no mandatory ESPressio dependency.

### Sockets → Event — opt-in

UDP, TCP, TLS, WebSocket and MQTT Event Transport adapters implement the ESPressio Event transport abstraction. The validated 0.2.3 baseline is Event 5.7.1 within the 5.x line.

### Sockets → Timing — opt-in

Socket/SNTP System Clock synchronization implementations consume ESPressio Timing. The validated 0.2.3 baseline is Timing 2.2.2 within the 2.x line.

---

## ESPressio Serial 0.3.3

**Required ESPressio dependencies: none.**

The core Serial, Console, logging and diagnostic foundations deliberately remain usable without mandatory ESPressio dependencies.

Optional integrations are independently selected:

- System Clock Monitor → Timing `>=2.2.2 <3.0.0`;
- Thread Monitor → Threads `>=3.1.2 <4.0.0`;
- Event Monitor/EventConsole → Event `>=5.7.1 <6.0.0`;
- Serializable payload/JSON facilities used by Event tooling → Serializable `>=0.10.0` within the compatible 0.x integration line.

---

## Required-dependency hierarchy

```text
FOUNDATIONAL
├── ESPressio Units 0.2.1
├── ESPressio Observable 3.0.1
├── ESPressio Serializable 0.10.0
├── ESPressio Sockets 0.2.3
└── ESPressio Serial 0.3.3

RUNTIME
└── ESPressio Timing 2.2.2
    ├── Units >=0.2.1 <1.0.0
    └── Observable >=3.0.1 <4.0.0

EXECUTION / COMMUNICATION
├── ESPressio Threads 3.1.2
│   ├── Timing >=2.2.2 <3.0.0
│   └── Observable >=3.0.1 <4.0.0
│
└── ESPressio ESP-Now 0.2.3
    └── Timing >=2.2.2 <3.0.0

EVENT INFRASTRUCTURE
└── ESPressio Event 5.7.1
    ├── Threads >=3.1.2 <4.0.0
    ├── Timing >=2.2.2 <3.0.0
    └── Observable >=3.0.1 <4.0.0
```

## Opt-in cross-cutting relationships

```text
Units
  - - -> Serializable
         Serializable Unit variants

Event
  - - -> Serializable
         Serializable Events / runtime construction / Event Transport

ESP-Now
  - - -> Event
         ESP-NOW Event Transport

Sockets
  - - -> Event
         socket Event Transports

Sockets
  - - -> Timing
         socket/SNTP clock synchronization

Serial
  - - -> Timing
         System Clock Monitor
  - - -> Threads
         Thread Monitor
  - - -> Event
         Event Monitor / EventConsole
  - - -> Serializable
         Event payload representation / JSON composition
```

---

## Dependency-refresh release order

When only dependency baselines change, downstream patch releases must consume the already-bumped upstream patch release rather than the pre-refresh version.

For the 2026-08-20 cascade the release order is:

```text
Observable 3.0.1      already released
Serializable 0.10.0   already released
        |
        v
Timing 2.2.2
        |
        v
Threads 3.1.2
        |
        v
Event 5.7.1
        |
        +--> ESP-Now 0.2.3
        +--> Sockets 0.2.3
        +--> Serial 0.3.3
```

This ensures every downstream release advertises the newest upstream patch generation that existed when that downstream release was validated.

---

## Architectural principle

The dependency graph follows a general ESPressio design rule:

> A foundational library should expose the synchronous or transport-neutral abstraction it owns; higher-level integration libraries should opt into that abstraction rather than forcing the foundational library to depend upward.

Examples include:

```text
Timing
    exposes Observer notifications

Event
    optionally bridges those notifications into Events
```

```text
Timing
    exposes transport-neutral clock synchronization

ESP-Now / Sockets
    provide concrete synchronization transports
```

```text
Event
    exposes transport-neutral Event Transport

ESP-Now / Sockets
    provide concrete Event transports
```

```text
Units
    provides ordinary Unit types

Units + Serializable
    optionally provides Serializable Unit variants
```

```text
Serial
    provides dependency-free core console/logging facilities

Serial + selected ESPressio integrations
    adds monitors and runtime Event tooling only when requested
```

This keeps the individual libraries independently useful while allowing progressively richer ESPressio compositions without imposing unnecessary dependencies on applications that do not use those integrations.
