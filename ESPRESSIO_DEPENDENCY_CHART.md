# ESPressio Dependency Chart — Event 5.8.4

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

## Required dependencies

```text
ESPressio Event 5.8.4
    -> ESPressio Threads >= 3.1.4 < 4.0.0
    -> ESPressio Timing >= 2.2.4 < 3.0.0
    -> ESPressio Observable >= 3.0.1 < 4.0.0
```

The transitive required chain is:

```text
Event 5.8.4
    -> Threads 3.1.4
        -> Timing 2.2.4
            -> Units 0.2.3
        -> Observable 3.0.1
    -> Timing 2.2.4
        -> Units 0.2.3
        -> Observable 3.0.1
    -> Observable 3.0.1
```

## Optional Serializable integration

Ordinary local Event use does not require Serializable. Serializable Events,
runtime Serializable Event construction, and Event Transport require:

```text
ESPressio Serializable >= 0.10.2 < 1.0.0
```

Event 5.8.4 preserves the allocation-free Event lifecycle synchronization fix
released in 5.8.3. The Event Transport envelope and ESPB v2 payload
representation remain unchanged.

## Optional Observer-to-Event bridges

Event 5.8.x provides opt-in bridges for Observable contracts from:

```text
Security >= 0.2.0 < 1.0.0
Command >= 0.3.0 < 1.0.0
Sockets >= 0.5.0 < 1.0.0
ESP-Now >= 0.5.3 < 1.0.0
```

The ESP-Now bridge baseline is validated against released ESPressio ESP-Now
0.5.3, including its peer-liveness reliability improvements. These integrations
remain optional.

## Current coordinated ecosystem

```text
FOUNDATIONAL
├── Observable 3.0.1
├── Serializable 0.10.2
├── Units 0.2.3
├── Security 0.2.0
└── Command 0.3.0

RUNTIME
└── Timing 2.2.4

EXECUTION
└── Threads 3.1.4

TRANSPORT / INTEGRATION
├── Sockets 0.5.0
└── ESP-Now 0.5.3

EVENT
└── Event 5.8.4

DIAGNOSTICS / OPERATOR
└── Serial 0.5.1
    (0.5.2 cascade pending Event 5.8.4 release)
```

## Circular-dependency audit

There are two reciprocal optional relationships that should be removed in a
future architecture cleanup:

```text
ESP-Now - - -> Event
    ESPNowEventTransport

Event - - -> ESP-Now
    ESPNowTransportEventBridge
```

and:

```text
Sockets - - -> Event
    socket Event transports

Event - - -> Sockets
    SocketWorkerEventBridge
    SocketSecuritySessionEventBridge
```

Although none of these edges is mandatory, each pair violates the desired
dependency rule: integration dependencies should cascade downstream and must not
point back into a consumer that already depends on the abstraction.

### Preferred resolution

Event should remain the upstream, transport-neutral abstraction. Transport-
specific Observer-to-Event bridges should move downstream:

```text
Event
  ^
  |
  | optional
  |
ESP-Now Event integration
  ├── ESPNowEventTransport
  └── ESPNowTransportEventBridge

Sockets Event integration
  ├── socket Event transports
  ├── SocketWorkerEventBridge
  └── SocketSecuritySessionEventBridge
```

Dedicated integration packages would also be valid if keeping the core
libraries completely unaware of one another is preferable.

Until those relocations occur, Event 5.8.4 does not add or strengthen any new
Event -> ESP-Now or Event -> Sockets package-level dependency. Existing bridge
headers remain available for compatibility only.

## Dependency-direction rule

> A library should expose the abstraction it owns. Integration code that adds a
> new dependency belongs downstream of that abstraction.

Examples:

```text
Timing owns clock discipline
    -> ESP-Now / Sockets provide concrete synchronization transports

Event owns Event semantics and transport abstraction
    -> ESP-Now / Sockets provide concrete Event transports

Observable libraries own synchronous lifecycle contracts
    -> Event can bridge them only when doing so does not create a reverse edge
```
