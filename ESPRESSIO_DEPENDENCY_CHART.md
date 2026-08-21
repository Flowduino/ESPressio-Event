# ESPressio Dependency Chart — Event 6.0.0

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

## Event 6.0.0 dependency model

```text
ESPressio Event 6.0.0
    -> ESPressio Threads >= 3.1.4 < 4.0.0
    -> ESPressio Timing >= 2.2.4 < 3.0.0
    -> ESPressio Observable >= 3.0.1 < 4.0.0
    - - -> ESPressio Serializable >= 0.10.2 < 1.0.0
            Serializable Events / Event Transport only
```

Event 6.0.0 is mechanism-only. It no longer contains concrete Command,
Security, Sockets, or ESP-Now Event types or Observer-to-Event bridges.

Timing and Threads bridges remain in Event because Event already consumes those
libraries directly for its own core responsibilities; Event is therefore the
legitimate lower-order consumer in those relationships.

## Final coordinated ecosystem

```text
FOUNDATIONAL / DOMAIN
├── Observable 3.0.1
├── Serializable 0.10.2
├── Units 0.2.3
├── Security 0.3.0
└── Command 0.4.0

RUNTIME
└── Timing 2.2.4
    ├── Units >= 0.2.3 < 1.0.0
    └── Observable >= 3.0.1 < 4.0.0

EXECUTION
└── Threads 3.1.4
    ├── Timing >= 2.2.4 < 3.0.0
    └── Observable >= 3.0.1 < 4.0.0

EVENT MECHANISM
└── Event 6.0.0
    ├── Threads >= 3.1.4 < 4.0.0
    ├── Timing >= 2.2.4 < 3.0.0
    ├── Observable >= 3.0.1 < 4.0.0
    └ - - Serializable >= 0.10.2 < 1.0.0

DOMAIN EVENT INTEGRATIONS
├── Command 0.4.0 - - -> Event 6.0.0
├── Security 0.3.0 - - -> Event 6.0.0
├── Sockets 0.6.0 - - -> Event 6.0.0
└── ESP-Now 0.6.0 - - -> Event 6.0.0

DIAGNOSTICS / OPERATOR
└── Serial 0.6.0
```

## Bridge ownership

Concrete Event types and Observer-to-Event bridges belong in the lowest-order
library whose concepts they represent, provided that placement does not create a
reverse dependency.

```text
Event 6.0.0
    owns generic Event mechanism
    owns Timing/SystemClock bridges
    owns Threads infrastructure bridges

Command 0.4.0
    owns Command Event types / CommandRegistryEventBridge

Security 0.3.0
    owns Security Event types / TransportSecurityEventBridge

Sockets 0.6.0
    owns Socket Event types / SocketWorkerEventBridge /
    SocketSecuritySessionEventBridge

ESP-Now 0.6.0
    owns ESP-Now Event types / ESPNowTransportEventBridge /
    ESPNowEventTransport
```

## Circular-dependency status

After this architecture change there are no intended reciprocal Event/domain
relationships:

```text
Event -> Command    NONE
Event -> Security   NONE
Event -> Sockets    NONE
Event -> ESP-Now    NONE

Command  - - -> Event
Security - - -> Event
Sockets  - - -> Event
ESP-Now  - - -> Event
```

Event's direct required relationships to Timing, Threads, and Observable are
retained because Event's generic implementation consumes those APIs directly.
Serializable remains optional and one-way.

The dependency-boundary CI check rejects future source or package-level reverse
dependencies from Event to Command, Security, Sockets, or ESP-Now.
