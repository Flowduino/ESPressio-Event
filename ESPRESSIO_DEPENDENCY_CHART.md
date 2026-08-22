# ESPressio Dependency Chart — Current Released Generation

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

This document is the canonical snapshot of the current released ESPressio dependency generation. Arrows point from the consuming library to the library it consumes.

- **Required** — the dependency is part of the library's normal/core contract.
- **Opt-in** — the dependency is introduced only when the corresponding integration/header is selected.

## Released generation

```text
Observable    3.0.1
Serializable  0.10.2
Units         0.2.3
Timing        2.2.4
Threads       3.1.4
Event         6.0.0
Command       1.0.0
Security      0.3.0
Sockets       0.7.0
ESP-Now       0.7.0
Serial        0.7.1
```

## Required dependencies

```text
Observable 3.0.1
    -> none

Serializable 0.10.2
    -> none

Units 0.2.3
    -> none

Timing 2.2.4
    -> Units >= 0.2.3 < 1.0.0
    -> Observable >= 3.0.1 < 4.0.0

Threads 3.1.4
    -> Timing >= 2.2.4 < 3.0.0
    -> Observable >= 3.0.1 < 4.0.0

Event 6.0.0
    -> Threads >= 3.1.4 < 4.0.0
    -> Timing >= 2.2.4 < 3.0.0
    -> Observable >= 3.0.1 < 4.0.0

Command 1.0.0
    -> Observable >= 3.0.1 < 4.0.0

Security 0.3.0
    -> Observable >= 3.0.1 < 4.0.0

Sockets 0.7.0
    -> Observable >= 3.0.1 < 4.0.0

ESP-Now 0.7.0
    -> Timing >= 2.2.4 < 3.0.0
    -> Observable >= 3.0.1 < 4.0.0

Serial 0.7.1
    -> none in the core package
```

## Opt-in integrations

```text
Units
    - - -> Serializable >= 0.10.2 < 1.0.0
            Serializable Unit variants

Event
    - - -> Serializable >= 0.10.2 < 1.0.0
            Serializable Events / Event Transport

Command
    - - -> Event >= 6.0.0 < 7.0.0
            Command-owned Event types / CommandRegistryEventBridge

Security
    - - -> Event >= 6.0.0 < 7.0.0
            Security-owned Event types / TransportSecurityEventBridge

Sockets
    - - -> Event >= 6.0.0 < 7.0.0
    - - -> Command >= 1.0.0 < 2.0.0
    - - -> Security >= 0.3.0 < 1.0.0
    - - -> Timing >= 2.2.4 < 3.0.0

ESP-Now
    - - -> Event >= 6.0.0 < 7.0.0
    - - -> Command >= 1.0.0 < 2.0.0
    - - -> Security >= 0.3.0 < 1.0.0

Serial
    - - -> Command >= 1.0.0 < 2.0.0
    - - -> Security >= 0.3.0 < 1.0.0
    - - -> Sockets >= 0.7.0 < 1.0.0
    - - -> ESP-Now >= 0.7.0 < 1.0.0
    - - -> Event >= 6.0.0 < 7.0.0
    - - -> Serializable >= 0.10.2 < 1.0.0
    - - -> Timing >= 2.2.4 < 3.0.0
    - - -> Threads >= 3.1.4 < 4.0.0
```

`JsonCommandInterpreter` optionally consumes external **ArduinoJson 7.x**. ArduinoJson is not an ESPressio library and is therefore not represented as an ESPressio graph edge.

## Dependency-direction invariants

Event 6.0.0 owns the generic Event mechanism. Domain-specific Event types and bridges belong to the lowest-order library that owns the represented concept without introducing a reverse dependency:

```text
Command  - - -> Event
Security - - -> Event
Sockets  - - -> Event
ESP-Now  - - -> Event

Event -> Command   NONE
Event -> Security  NONE
Event -> Sockets   NONE
Event -> ESP-Now   NONE
```

Timing and Threads Event bridges remain in Event because Event already requires Timing and Threads for its own responsibilities; moving those bridges upstream would create reverse dependencies.

Serial remains terminal/downstream. No upstream ESPressio library should depend on Serial.

## Standalone repositories

ESPressio Tree and ESPressio WiFi are not dependency edges in the coordinated graph above. Tree is a standalone generic component. WiFi currently has no implemented public API and must not be treated as a dependency of the released stack merely because legacy package metadata exists in its repository.
