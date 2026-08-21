# Changelog

## 5.8.3 — 2026-08-21

### Fixed
- Removed the two per-instance `Threads::ReadWriteMutex` / `std::shared_mutex` objects from `Event<T>` lifecycle metadata to prevent repeated pthread/FreeRTOS rwlock allocation under sustained Event churn on ESP32.
- Replaced those allocation-backed locks with per-Event allocation-free synchronization while preserving first-dispatch timestamp semantics and coherent `EventDispatchContext` access.
- Fixed the hardware-observed abort path that reached `pthread_rwlock_init()` / ESP-IDF `lock_init_generic()` from `EventManager::OnEventDispatched()` during long-running EventConsole-Lab workloads.

### Changed
- Updated package metadata, README, and dependency documentation for Event 5.8.3.
- Kept the required dependency baselines unchanged at Threads `>=3.1.4 <4.0.0`, Timing `>=2.2.4 <3.0.0`, and Observable `>=3.0.1 <4.0.0`.
- Kept Serializable `>=0.10.2 <1.0.0` as the validated optional baseline for Serializable Events and Event Transport.

### Compatibility
- No Event, EventThread, listener, Observer, Serializable Event, Event Transport routing, or wire-format API changes are introduced by this patch.
- The Event Transport envelope and ESPB v2 payload representation are unchanged.

## 5.8.2 — 2026-08-21

### Changed
- Raised the required ESPressio Threads baseline from 3.1.3 to 3.1.4.
- Raised the required ESPressio Timing baseline from 2.2.3 to 2.2.4.
- Raised the optional ESPressio Serializable baseline from 0.10.1 to 0.10.2 for Serializable Events, runtime construction and Event Transport integrations.
- Preserved the required ESPressio Observable baseline at `>=3.0.1 <4.0.0`.
- Updated package, ESP-IDF component, README and dependency documentation for Event 5.8.2.
- Preserved Security, Command, Sockets and ESP-Now as opt-in bridge dependencies and did not strengthen the existing Event↔ESP-Now or Event↔Sockets reciprocal optional relationships.

### Compatibility
- No Event, EventThread, listener, Observer, Event Transport, routing, bridge or wire-format API changes are introduced by this dependency-maintenance release.
- Event Transport continues to use the existing ESPB v2 payload representation.

## 5.8.1 — 2026-08-20

### Changed
- Raised the required ESPressio Threads baseline from 3.1.2 to 3.1.3.
- Raised the required ESPressio Timing baseline from 2.2.2 to 2.2.3.
- Raised the optional ESPressio Serializable baseline from 0.10.0 to 0.10.1 for Serializable Events, runtime construction and Event Transport integrations.
- Preserved the required ESPressio Observable baseline at `>=3.0.1 <4.0.0`.
- Updated package and ESP-IDF component metadata for Event 5.8.1.
- Documented the reciprocal optional Event/ESP-Now dependency as an architectural exception: the preferred long-term resolution is to relocate `ESPNowTransportEventBridge` downstream into ESP-Now's Event integration (or a dedicated integration package), keeping Event transport-neutral.

### Compatibility
- No Event, EventThread, listener, Observer, Event Transport, routing, bridge or wire-format API changes are introduced by this dependency-maintenance release.
- Security, Command, Sockets, ESP-Now and Serializable remain opt-in where applicable.

## 5.8.0 — 2026-08-20

### Added
- Added opt-in `TransportSecurityEventBridge` and Security lifecycle Event types for ESPressio Security 0.2.x configuration, session, replay-protection and failure observations.
- Added opt-in `CommandRegistryEventBridge` and Command registry Event types for ESPressio Command 0.3.x registration lifecycle observations.
- Added opt-in `SocketWorkerEventBridge` and `SocketSecuritySessionEventBridge` with corresponding Sockets lifecycle Event types for ESPressio Sockets 0.5.x.
- Added opt-in `ESPNowTransportEventBridge` and ESP-NOW lifecycle Event types for ESPressio ESP-Now 0.5.x initialization, peer and send observations.

### Changed
- Preserved the existing dependency direction: Security, Command, Sockets and ESP-Now remain independent of ESPressio Event; their Event bridges are selected explicitly from ESPressio Event.
- Kept Event's mandatory dependency set unchanged at Threads 3.1.2+, Observable 3.0.1+ and Timing 2.2.2+ within their current major lines.
- Updated package and ESP-IDF component version metadata for Event 5.8.0.

### Compatibility
- Existing Event, EventThread, Event Transport, Timing bridges and Thread bridges remain source-compatible.
- No new mandatory Security, Command, Sockets or ESP-Now dependency is introduced.

## 5.7.1 — 2026-08-20

### Changed
- Raised the required ESPressio Threads dependency floor from 3.1.1 to 3.1.2.
- Raised the required ESPressio Timing dependency floor from 2.2.1 to 2.2.2.
- Retained the direct ESPressio Observable minimum at 3.0.1 and the optional Serializable integration baseline at 0.10.0.
- Updated package and ESP-IDF component version metadata for Event 5.7.1.

### Compatibility
- This is a dependency-maintenance patch release only.
- No Event, EventThread, Listener, Observer, Event Transport, runtime Serializable Event, Event Bridge, routing, or wire-protocol interfaces changed.

## 5.7.0 — 2026-08-20

### Added
- Added compile-time task scheduling configuration through `ESPRESSIO_EVENT_MANAGER_PRIORITY`, `ESPRESSIO_EVENT_MANAGER_CORE_ID`, `ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY`, `ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID`, `ESPRESSIO_EVENT_TRANSPORT_MANAGER_PRIORITY`, and `ESPRESSIO_EVENT_TRANSPORT_MANAGER_CORE_ID`.
- Added CI coverage for host Event tests and a real ESP32/Arduino PlatformIO compile of EventThread and Event Transport.

### Changed
- Replaced EventManager, EventThread, and EventTransportManager binary-semaphore wakeups with FreeRTOS direct task notifications.
- Replaced EventReceiver priority `unordered_map` storage with fixed indexed priority buckets.
- Replaced per-dispatch receiver-vector copies with immutable shared receiver snapshots; receiver vectors are now copied only when registrations change.
- Updated the required ESPressio Observable baseline to 3.0.1 to consume the zero-Observer notification fast path.
- Event Transport now uses ESPressio Serializable 0.10.0 direct Binary serialization/deserialization when its transport facilities are selected.
- Event Transport outbound packet construction now allocates the final envelope+payload buffer once and serializes directly after the envelope, removing the previous intermediate payload vector and payload copy.
- Direct transport decoding preserves the existing BinaryArchive path as a fallback for schema migration and compatibility cases.

### Compatibility
- This is a backward-compatible public interface extension.
- Existing `Queue()` and `Stack()` asynchronous dispatch semantics are unchanged.
- Event priority ordering, bounded receiver queues, registration/unregistration safety, local/remote origin handling, and routing semantics are preserved.
- Event Transport continues to use the existing EVTT envelope version and the existing BinaryArchive ESPB v2 payload representation; no wire-protocol version bump is required.

## 5.6.2 — 2026-08-19

### Changed
- Updated active ESPressio dependency baselines to the latest released versions available on 2026-08-19.
- Bounded dependency compatibility to the current major version so future breaking major releases are not selected automatically.
- Updated the required ESPressio Threads baseline to 3.1.1 and ESPressio Timing baseline to 2.2.1, consuming the dependency-refresh patch releases produced earlier in the release chain.

All notable changes to this project are documented in this file.

The structure follows the principles of [Keep a
Changelog](https://keepachangelog.com/en/1.1.0/) and [Semantic
Versioning](https://semver.org/).

> **Historical note:** This changelog was reconstructed retrospectively
> from published GitHub Releases, tags, release notes, repository
> history, and the documented public API. Where an historical release
> had little or no release-note detail, the entry is intentionally terse
> rather than inferring unsupported intent.

## [5.6.1] - 2026-08-19

### Fixed

- Added explicit C++17 equality semantics to `EventDispatchContext`.
- Fixed compilation when `EventDispatchContext` is used with ESPressio Threads 3.1 `ReadWriteMutex<T>` change detection, whose default comparator requires `T` to support `operator==`.
- Equality now compares the complete dispatch-context identity: `Origin`, `TransportMessageID`, and `HopCount`.
- Added regression coverage for equal contexts and for differences in each individual context field.

### Compatibility

- This is a backward-compatible patch release.
- No existing Event, listener, Observer, Event Transport, runtime Serializable Event, or Event Bridge interfaces were removed or changed.

## [5.6.0] - 2026-08-19

### Added

- Added public runtime discovery for registered Serializable Event types through `GetRegisteredSerializableEvents()`.
- Added runtime lookup by stable Event type name or stable type ID through `FindRegisteredSerializableEvent(...)`.
- Added immutable `SerializableEventDescriptor` snapshots exposing stable type identity, schema version, default transport direction, property schema metadata, and runtime constructibility.
- Added type-erased `SerializationNode` → concrete Serializable Event construction through `CreateSerializableEvent(...)`.
- Added `SerializableEventConstructionResult`, preserving ESPressio Serializable `DeserializationResult` validation and migration diagnostics.
- Added ownership-safe type-erased Queue/Stack dispatch through `DispatchSerializableEvent(...)`.
- Added runtime schema capture using ESPressio Serializable `SchemaInspector<TEvent>`.
- Added `RuntimeSerializableEvents` example demonstrating discovery, schema inspection, representation-neutral construction, validation diagnostics, and runtime dispatch.

### Changed

- Extended Event Transport registration metadata with type-erased runtime construction and schema information.
- Runtime construction deliberately consumes `SerializationNode` rather than JSON, keeping ESPressio Event independent of ArduinoJson and allowing Serial, REST, WebSocket, MQTT, tests, and replay tooling to choose their own input representation.
- Existing Event registration, dispatch, listener, transport, and Observer APIs remain source-compatible.

### Compatibility

- This is a backward-compatible public interface extension; no existing interfaces were removed or changed.
- ESPressio Serializable 0.9.0 remains the minimum dependency for Event Transport/runtime Serializable Event facilities.

## [5.5.0] - 2026-08-19

### Added

- Added **Event Transport Transaction Observation** as a unified diagnostics/tracing surface.
- Added `EventTransportTransactionStage`.
- Added immutable callback snapshots through `EventTransportTransaction`.
- Added `IEventTransportManagerObserver::OnEventTransportTransaction(...)`.
- Added stable human-readable Event transport type names to runtime registrations and transaction snapshots.
- Added transaction-stage notifications for outbound acceptance, serialization, transport handoff, inbound acceptance/rejection, deserialization, local dispatch, and processing failures.
- Exposed borrowed Event and Binary payload context at transaction stages where those representations exist.

### Changed

- Extended the Event Transport Observer interface without removing or changing existing Observer callbacks.
- Documented Event/payload pointers in transaction snapshots as callback-lifetime-only borrowed references.
- Kept transaction observation representation-neutral; Event itself does not acquire JSON/diagnostic-output dependencies.

### Fixed

- Corrected `EventTransportManager::Initialize()` for ESPressio Threads 3.1 by returning `ThreadInitializationStatus` as required by the base `Thread` interface.
- Updated Event Transport Manager startup state handling to use the current Threads `Running` state rather than the obsolete `Started` state.

## [5.4.0] - 2026-08-19

### Added

- Added per-transport Event routing policy.
- Added transport-specific registration and unregistration overloads.
- Added C++17 variadic bulk registration/unregistration for specific transports.
- Added transport-specific `UnregisterAll...()` operations.
- Added independent pending-work handling per concrete transport.
- Added explicit per-transport route overrides while preserving global/default routing policy.

### Changed

- Extended the multi-transport architecture introduced in 5.3 so one Serializable Event type can have different inbound/outbound policy on different transports.

## [5.3.0] - 2026-08-19

### Added

- Added transport-neutral bidirectional Serializable Event transport architecture.
- Added `EventTransportManager`.
- Added `IEventTransport` abstraction for concrete transports implemented by other libraries.
- Added inbound, outbound, and bidirectional Event-type registration.
- Added C++17 variadic bulk registration.
- Added Event-type unregistration and pending inbound/outbound work policies.
- Added stable Event wire identities, transport envelopes, message IDs, dispatch metadata, and hop information.
- Added remote-origin tracking and default loop prevention.
- Added Event Transport Manager Observer diagnostics.

## [5.2.0] - 2026-08-19

### Added

- Added opt-in Observer-to-Event bridges for ESPressio Threads singleton infrastructure.
- Added Thread Manager, Thread Garbage Collector, and Thread Termination Dispatcher Event families.
- Added Serializable counterparts for Thread infrastructure Events and bridges.
- Added grouped `thread-events` headers and batch bridge headers.

### Changed

- Preserved the dependency direction whereby Threads exposes synchronous Observers and Event optionally converts them to asynchronous Events.

## [5.1.0] - 2026-08-18

### Added

- Added `SystemClockEventBridge`.
- Added strongly typed Timing Events corresponding to ESPressio Timing 2.2 System Clock Observer callbacks.
- Added Serializable Timing Event counterparts.
- Added `SerializableSystemClockEventBridge`.
- Added `src/timing-events` organization and `ESPressio_TimingEvents.hpp` batch header.

### Changed

- Made Timing-to-Event conversion explicitly opt-in; Timing remains independent of Event.

## [5.0.1] - 2026-08-18

### Fixed

- Corrected compilation failures in `ESPressio_EventListener.hpp` introduced during the 5.0 migration.
- Corrected related template/interface integration issues discovered by full compile analysis.
- Bumped the patch version without changing the 5.x architecture.

## [5.0.0] - 2026-08-18

### Added

- Added generic `Event<TTime>` public time representation.
- Added optional `SerializableEvent<TDerived, TTime>`.
- Added opt-in Serializable Event headers while keeping ordinary Events serialization-free.
- Added generic PrecisionEventThread timing integration with Timing 2.x.

### Changed

- Migrated Event to the current ESPressio Threads 3.x / Timing 2.x architecture.
- Kept the type-erased routing engine operating through `IEvent*` and raw nanosecond lifecycle timing.
- Separated public typed Event time values from internal timing storage.

## [4.0.0] - 2026-08-13

### Added

- Added `EventListenerHandlePtr` as RAII ownership for Event listener registrations.
- Added bounded Event receiver queues and retained-Event limits.
- Added Event queue statistics and retained-capacity policies.

### Changed

- Changed listener/Observer registration APIs from raw owning handles to smart-pointer ownership.
- Updated examples and documentation to use RAII listener handles.

### Fixed

- Removed listener-handle leak/ownership hazards.
- Prevented unbounded producer-over-consumer Event retention from exhausting embedded heap.
- Tightened exception safety and retained-reference accounting.

## [3.1.0]

### Changed

- Continued hardening and integration of the Event 3.x listener/observer architecture.

## [3.0.2]

### Fixed

- Patch-level corrections to the Event 3.0 architecture.

## [3.0.1]

### Fixed

- Initial corrective maintenance following Event 3.0.0.

## [3.0.0]

### Changed

- Established the Event 3.x architecture and dependency baseline used before the RAII-focused 4.0 migration.

## [2.1.0] - 2026-08-11

### Added

- Added ESPressio Observable 2.0 integration.
- Added typed `IEventObserver<EventType>`.
- Added `RegisterObserver<EventType>()`.
- Added Observer interest filtering for All, YoungerThan, and Custom policies.
- Added coexistence of callback listeners and typed Observers.
- Added host-side CMake/CTest coverage for Observer delivery, filtering, registration lifetime, routing, and exception paths.

### Changed

- Reworked listener dispatch through a type-erased virtual processing operation with checked Event casts.
- Changed dispatch to use stable listener snapshots so callbacks can safely register/unregister listeners.
- Improved EventManager registration/unregistration lifecycle.

### Fixed

- Corrected unsafe listener-container casts.
- Fixed callback-time deadlock/use-after-free risks.
- Fixed Event reference cleanup when callbacks/Observers throw.
- Fixed EventManager routing lifetime and dangling receiver risks.
- Fixed registration exception-safety and listener-bucket cleanup.

## [1.0.0] - 2026-02-24

### Added

- Initial release; core Event dispatch functionality tested and operational.
