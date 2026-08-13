#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_IEvent.hpp"

namespace ESPressio {
    namespace Event {

        enum class EventQueueOverflowPolicy : uint8_t {
            BlockProducer,
            RejectIncoming,
            DropOldest,
            DropLowestPriority
        };

        enum class EventCollectionCapacityPolicy : uint8_t {
            Retain,
            ShrinkWhenUnderutilized,
            ReleaseAfterDrain
        };

        class IEventReceiver {
            public:
                virtual ~IEventReceiver() = default;
                virtual void QueueEvent(
                    IEvent* event,
                    EventPriority priority = EventPriority::Normal
                ) = 0;
                virtual void StackEvent(
                    IEvent* event,
                    EventPriority priority = EventPriority::Normal
                ) = 0;
        };

        class EventReceiver : public IEventReceiver {
            private:
                struct PendingEvent {
                    IEvent* event = nullptr;
                    uint64_t sequence = 0;
                };

                using EventDispatchCollection = std::vector<PendingEvent>;
                using EventCollection = std::unordered_map<
                    EventPriority, EventDispatchCollection
                >;

                static constexpr size_t CapacitySampleCount = 16;

                mutable std::mutex _eventsMutex;
                std::condition_variable _capacityAvailable;
                EventCollection _priorityQueues;
                EventCollection _priorityStacks;
                size_t _pendingEventCount = 0;
                size_t _peakPendingEventCount = 0;
                size_t _maximumPendingEventCount = 0;
                EventQueueOverflowPolicy _overflowPolicy =
                    EventQueueOverflowPolicy::BlockProducer;
                EventCollectionCapacityPolicy _capacityPolicy =
                    EventCollectionCapacityPolicy::ShrinkWhenUnderutilized;
                size_t _minimumRetainedCapacity = 4;
                size_t _capacityExcessFactor = 2;
                std::array<size_t, CapacitySampleCount> _recentDrainSizes{};
                size_t _recentDrainIndex = 0;
                size_t _recentDrainCount = 0;
                uint64_t _nextSequence = 0;
                uint64_t _rejectedEventCount = 0;
                uint64_t _droppedEventCount = 0;
                bool _acceptingPendingEvents = true;

                static uint64_t NextSequence(uint64_t& sequence) {
                    const uint64_t result = sequence;
                    if (sequence != std::numeric_limits<uint64_t>::max()) {
                        ++sequence;
                    }
                    return result;
                }

                void RecordDrainSizeLocked(size_t size) {
                    _recentDrainSizes[_recentDrainIndex] = size;
                    _recentDrainIndex =
                        (_recentDrainIndex + 1) % CapacitySampleCount;
                    _recentDrainCount = std::min(
                        _recentDrainCount + 1, CapacitySampleCount
                    );
                }

                size_t RecentPeakLocked() const {
                    size_t peak = 0;
                    for (size_t index = 0;
                        index < _recentDrainCount; ++index) {
                        peak = std::max(peak, _recentDrainSizes[index]);
                    }
                    return peak;
                }

                void ApplyCapacityPolicyLocked(
                    EventDispatchCollection& collection
                ) {
                    if (_capacityPolicy ==
                        EventCollectionCapacityPolicy::Retain) {
                        return;
                    }
                    if (_capacityPolicy ==
                        EventCollectionCapacityPolicy::ReleaseAfterDrain) {
                        EventDispatchCollection replacement(collection);
                        replacement.shrink_to_fit();
                        collection.swap(replacement);
                        return;
                    }

                    const size_t recentPeak = RecentPeakLocked();
                    const size_t target = std::max(
                        _minimumRetainedCapacity,
                        recentPeak > std::numeric_limits<size_t>::max() /
                            _capacityExcessFactor
                            ? std::numeric_limits<size_t>::max()
                            : recentPeak * _capacityExcessFactor
                    );
                    if (collection.capacity() > target) {
                        EventDispatchCollection replacement;
                        replacement.reserve(std::max(
                            target, collection.size()
                        ));
                        replacement.insert(
                            replacement.end(),
                            std::make_move_iterator(collection.begin()),
                            std::make_move_iterator(collection.end())
                        );
                        collection.swap(replacement);
                    }
                }

                PendingEvent RemoveOldestLocked() {
                    EventDispatchCollection* selected = nullptr;
                    size_t selectedIndex = 0;
                    uint64_t selectedSequence =
                        std::numeric_limits<uint64_t>::max();
                    auto consider = [&](EventCollection& collections) {
                        for (auto& entry : collections) {
                            for (size_t index = 0;
                                index < entry.second.size(); ++index) {
                                if (entry.second[index].sequence <
                                    selectedSequence) {
                                    selected = &entry.second;
                                    selectedIndex = index;
                                    selectedSequence =
                                        entry.second[index].sequence;
                                }
                            }
                        }
                    };
                    consider(_priorityQueues);
                    consider(_priorityStacks);
                    PendingEvent removed = (*selected)[selectedIndex];
                    selected->erase(selected->begin() + selectedIndex);
                    --_pendingEventCount;
                    return removed;
                }

                PendingEvent RemoveLowestPriorityLocked(bool& removed) {
                    for (int priorityID = 0;
                        priorityID <= static_cast<int>(EventPriority::High);
                        ++priorityID) {
                        const EventPriority priority =
                            static_cast<EventPriority>(priorityID);
                        auto removeFrom = [&](EventCollection& collections) {
                            const auto found = collections.find(priority);
                            if (found == collections.end() ||
                                found->second.empty()) {
                                return PendingEvent{};
                            }
                            PendingEvent result = found->second.front();
                            found->second.erase(found->second.begin());
                            removed = true;
                            --_pendingEventCount;
                            return result;
                        };
                        PendingEvent result = removeFrom(_priorityQueues);
                        if (removed) {
                            return result;
                        }
                        result = removeFrom(_priorityStacks);
                        if (removed) {
                            return result;
                        }
                    }
                    return PendingEvent{};
                }

                void AddEvent(
                    IEvent* event,
                    EventPriority priority,
                    EventDispatchMethod method
                ) {
                    event->__dispatch();
                    event->__ref();
                    IEvent* displacedEvent = nullptr;
                    bool accepted = false;
                    try {
                        std::unique_lock<std::mutex> lock(_eventsMutex);
                        if (!_acceptingPendingEvents) {
                            ++_rejectedEventCount;
                            lock.unlock();
                            event->__unref();
                            return;
                        }
                        while (_maximumPendingEventCount > 0 &&
                            _pendingEventCount >=
                                _maximumPendingEventCount) {
                            if (!_acceptingPendingEvents) {
                                ++_rejectedEventCount;
                                lock.unlock();
                                event->__unref();
                                return;
                            }
                            switch (_overflowPolicy) {
                                case EventQueueOverflowPolicy::BlockProducer:
                                    _capacityAvailable.wait(lock, [&]() {
                                        return _maximumPendingEventCount == 0 ||
                                            _pendingEventCount <
                                                _maximumPendingEventCount ||
                                            !_acceptingPendingEvents ||
                                            _overflowPolicy !=
                                                EventQueueOverflowPolicy::
                                                    BlockProducer;
                                    });
                                    continue;
                                case EventQueueOverflowPolicy::RejectIncoming:
                                    ++_rejectedEventCount;
                                    lock.unlock();
                                    event->__unref();
                                    return;
                                case EventQueueOverflowPolicy::DropOldest:
                                    displacedEvent =
                                        RemoveOldestLocked().event;
                                    ++_droppedEventCount;
                                    break;
                                case EventQueueOverflowPolicy::
                                    DropLowestPriority: {
                                    bool removed = false;
                                    PendingEvent displaced =
                                        RemoveLowestPriorityLocked(removed);
                                    if (!removed) {
                                        ++_rejectedEventCount;
                                        lock.unlock();
                                        event->__unref();
                                        return;
                                    }
                                    displacedEvent = displaced.event;
                                    ++_droppedEventCount;
                                    break;
                                }
                            }
                            break;
                        }

                        EventCollection& collections =
                            method == EventDispatchMethod::Queue
                                ? _priorityQueues
                                : _priorityStacks;
                        collections[priority].push_back(PendingEvent{
                            event, NextSequence(_nextSequence)
                        });
                        ++_pendingEventCount;
                        _peakPendingEventCount = std::max(
                            _peakPendingEventCount, _pendingEventCount
                        );
                        accepted = true;
                    } catch (...) {
                        event->__unref();
                        if (displacedEvent != nullptr) {
                            displacedEvent->__unref();
                        }
                        throw;
                    }
                    if (displacedEvent != nullptr) {
                        displacedEvent->__unref();
                    }
                    if (accepted) {
                        EventAdded();
                    }
                }

                void ProcessCollection(
                    EventCollection& collections,
                    EventPriority priority,
                    EventDispatchMethod method,
                    const std::function<void(
                        IEvent*, EventDispatchMethod, EventPriority
                    )>& callback
                ) {
                    EventDispatchCollection pending;
                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        const auto found = collections.find(priority);
                        if (found == collections.end() ||
                            found->second.empty()) {
                            return;
                        }
                        pending.swap(found->second);
                        _pendingEventCount -= pending.size();
                        RecordDrainSizeLocked(pending.size());
                    }
                    _capacityAvailable.notify_all();

                    class PendingReferences final {
                        private:
                            EventDispatchCollection& _events;
                        public:
                            explicit PendingReferences(
                                EventDispatchCollection& events
                            ) : _events(events) { }
                            ~PendingReferences() {
                                for (PendingEvent& pending : _events) {
                                    if (pending.event != nullptr) {
                                        pending.event->__unref();
                                    }
                                }
                            }
                            void Release(size_t index) {
                                _events[index].event->__unref();
                                _events[index].event = nullptr;
                            }
                    } references(pending);

                    if (method == EventDispatchMethod::Stack) {
                        for (size_t index = pending.size(); index > 0; --index) {
                            const size_t current = index - 1;
                            callback(pending[current].event, method, priority);
                            references.Release(current);
                        }
                    } else {
                        for (size_t index = 0; index < pending.size(); ++index) {
                            callback(pending[index].event, method, priority);
                            references.Release(index);
                        }
                    }

                    pending.clear();
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    EventDispatchCollection& destination =
                        collections[priority];
                    if (pending.capacity() > destination.capacity()) {
                        EventDispatchCollection arrivals;
                        arrivals.swap(destination);
                        pending.swap(destination);
                        destination.insert(
                            destination.end(),
                            std::make_move_iterator(arrivals.begin()),
                            std::make_move_iterator(arrivals.end())
                        );
                    }
                    ApplyCapacityPolicyLocked(destination);
                }

            protected:
                void StopAcceptingEvents() noexcept {
                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        _acceptingPendingEvents = false;
                    }
                    _capacityAvailable.notify_all();
                }

                void WithEvents(
                    std::function<void(
                        IEvent*, EventDispatchMethod, EventPriority
                    )> callback
                ) {
                    for (int priorityID =
                            static_cast<int>(EventPriority::High);
                        priorityID >= 0; --priorityID) {
                        ProcessCollection(
                            _priorityStacks,
                            static_cast<EventPriority>(priorityID),
                            EventDispatchMethod::Stack,
                            callback
                        );
                    }
                    for (int priorityID =
                            static_cast<int>(EventPriority::High);
                        priorityID >= 0; --priorityID) {
                        ProcessCollection(
                            _priorityQueues,
                            static_cast<EventPriority>(priorityID),
                            EventDispatchMethod::Queue,
                            callback
                        );
                    }
                }

                void ClearPendingEvents() noexcept {
                    EventCollection queues;
                    EventCollection stacks;
                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        queues.swap(_priorityQueues);
                        stacks.swap(_priorityStacks);
                        _pendingEventCount = 0;
                    }
                    _capacityAvailable.notify_all();
                    auto release = [](EventCollection& collections) {
                        for (auto& entry : collections) {
                            for (PendingEvent& pending : entry.second) {
                                pending.event->__unref();
                            }
                        }
                    };
                    release(queues);
                    release(stacks);
                }

                virtual void EventAdded() { }

            public:
                ~EventReceiver() override {
                    StopAcceptingEvents();
                    ClearPendingEvents();
                }

                void QueueEvent(
                    IEvent* event,
                    EventPriority priority = EventPriority::Normal
                ) override {
                    AddEvent(event, priority, EventDispatchMethod::Queue);
                }

                void StackEvent(
                    IEvent* event,
                    EventPriority priority = EventPriority::Normal
                ) override {
                    AddEvent(event, priority, EventDispatchMethod::Stack);
                }

                size_t GetMaximumPendingEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _maximumPendingEventCount;
                }
                void SetMaximumPendingEventCount(size_t maximum) {
                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        _maximumPendingEventCount = maximum;
                    }
                    _capacityAvailable.notify_all();
                }
                EventQueueOverflowPolicy GetEventQueueOverflowPolicy() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _overflowPolicy;
                }
                void SetEventQueueOverflowPolicy(
                    EventQueueOverflowPolicy policy
                ) {
                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        _overflowPolicy = policy;
                    }
                    _capacityAvailable.notify_all();
                }
                EventCollectionCapacityPolicy
                GetEventCollectionCapacityPolicy() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _capacityPolicy;
                }
                void SetEventCollectionCapacityPolicy(
                    EventCollectionCapacityPolicy policy
                ) {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    _capacityPolicy = policy;
                    auto apply = [&](EventCollection& collections) {
                        for (auto& entry : collections) {
                            ApplyCapacityPolicyLocked(entry.second);
                        }
                    };
                    apply(_priorityQueues);
                    apply(_priorityStacks);
                }
                size_t GetPendingEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _pendingEventCount;
                }
                size_t GetPeakPendingEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _peakPendingEventCount;
                }
                size_t GetRetainedEventCapacity() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    size_t capacity = 0;
                    auto addCapacity = [&](const EventCollection& collections) {
                        for (const auto& entry : collections) {
                            capacity += entry.second.capacity();
                        }
                    };
                    addCapacity(_priorityQueues);
                    addCapacity(_priorityStacks);
                    return capacity;
                }
                uint64_t GetRejectedEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _rejectedEventCount;
                }
                uint64_t GetDroppedEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _droppedEventCount;
                }
                void ResetEventQueueStatistics() {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    _peakPendingEventCount = _pendingEventCount;
                    _rejectedEventCount = 0;
                    _droppedEventCount = 0;
                }
                size_t GetMinimumRetainedEventCapacity() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _minimumRetainedCapacity;
                }
                void SetMinimumRetainedEventCapacity(size_t capacity) {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    _minimumRetainedCapacity = capacity;
                }
                size_t GetEventCapacityExcessFactor() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _capacityExcessFactor;
                }
                void SetEventCapacityExcessFactor(size_t factor) {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    _capacityExcessFactor = std::max<size_t>(factor, 1);
                }
        };
    }
}
