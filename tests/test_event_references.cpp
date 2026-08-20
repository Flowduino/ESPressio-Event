#include <cassert>
#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <vector>

#include "ESPressio_EventDispatcher.hpp"

using namespace ESPressio::Event;

class ReferenceTrackingEvent final : public IEvent {
    private:
        int _references = 0;
        EventDispatchContext _dispatchContext{};

    public:
        void __ref() noexcept override { ++_references; }
        void __unref() noexcept override {
            assert(_references > 0);
            --_references;
        }
        void __dispatch() override { }
        void __setDispatchContext(const EventDispatchContext& context) override {
            _dispatchContext = context;
        }
        EventDispatchContext __getDispatchContext() const override {
            return _dispatchContext;
        }
        void Queue(EventPriority = EventPriority::Normal) override { }
        void Stack(EventPriority = EventPriority::Normal) override { }
        uint64_t GetDispatchTimeNanoseconds() const override { return 0; }
        uint64_t GetTimeSinceDispatchNanoseconds() const override { return 0; }
        int References() const { return _references; }
};

class TrackingReceiver final : public EventReceiver {
    public:
        std::vector<EventDispatchMethod> methods;

        void Drain() {
            WithEvents([&](
                IEvent* event,
                EventDispatchMethod method,
                EventPriority
            ) {
                methods.push_back(method);
                (void)event;
            });
        }

        void DrainWithoutRecording() {
            WithEvents([](
                IEvent*, EventDispatchMethod, EventPriority
            ) { });
        }

        void DrainWithFailure() {
            WithEvents([](
                IEvent*, EventDispatchMethod, EventPriority
            ) {
                throw std::runtime_error("expected processing failure");
            });
        }
};

class TestDispatcher final : public EventDispatcher {
    public:
        void Dispatch() { DispatchEvents(); }
};

class HeapTrackingEvent final : public IEvent {
    private:
        int _references = 0;
        int& _liveEvents;
        EventDispatchContext _dispatchContext{};

    public:
        explicit HeapTrackingEvent(int& liveEvents)
            : _liveEvents(liveEvents) {
            ++_liveEvents;
        }

        ~HeapTrackingEvent() { --_liveEvents; }

        void __ref() noexcept override { ++_references; }
        void __unref() noexcept override {
            assert(_references > 0);
            if (--_references == 0) {
                delete this;
            }
        }
        void __dispatch() override { }
        void __setDispatchContext(const EventDispatchContext& context) override {
            _dispatchContext = context;
        }
        EventDispatchContext __getDispatchContext() const override {
            return _dispatchContext;
        }
        void Queue(EventPriority = EventPriority::Normal) override { }
        void Stack(EventPriority = EventPriority::Normal) override { }
        uint64_t GetDispatchTimeNanoseconds() const override { return 0; }
        uint64_t GetTimeSinceDispatchNanoseconds() const override { return 0; }
};

int main() {
    ReferenceTrackingEvent dispatchedEvent;
    TrackingReceiver receiver;
    TestDispatcher dispatcher;

    dispatcher.RegisterReceiver(typeid(dispatchedEvent), &receiver);
    dispatcher.QueueEvent(&dispatchedEvent);
    assert(dispatchedEvent.References() == 1);
    dispatcher.Dispatch();
    assert(dispatchedEvent.References() == 1);
    receiver.Drain();
    assert(dispatchedEvent.References() == 0);
    dispatcher.UnregisterReceiver(typeid(dispatchedEvent), &receiver);

    ReferenceTrackingEvent unhandledEvent;
    dispatcher.QueueEvent(&unhandledEvent);
    dispatcher.Dispatch();
    assert(unhandledEvent.References() == 0);

    ReferenceTrackingEvent queuedEvent;
    ReferenceTrackingEvent stackedEvent;
    TrackingReceiver orderedReceiver;
    orderedReceiver.QueueEvent(&queuedEvent);
    orderedReceiver.StackEvent(&stackedEvent);
    orderedReceiver.Drain();
    assert(orderedReceiver.methods.size() == 2);
    assert(orderedReceiver.methods[0] == EventDispatchMethod::Stack);
    assert(orderedReceiver.methods[1] == EventDispatchMethod::Queue);
    assert(queuedEvent.References() == 0);
    assert(stackedEvent.References() == 0);

    ReferenceTrackingEvent abandonedQueueEvent;
    ReferenceTrackingEvent abandonedStackEvent;
    {
        TrackingReceiver abandonedReceiver;
        abandonedReceiver.QueueEvent(&abandonedQueueEvent);
        abandonedReceiver.StackEvent(&abandonedStackEvent);
        assert(abandonedQueueEvent.References() == 1);
        assert(abandonedStackEvent.References() == 1);
    }
    assert(abandonedQueueEvent.References() == 0);
    assert(abandonedStackEvent.References() == 0);

    ReferenceTrackingEvent failedEventOne;
    ReferenceTrackingEvent failedEventTwo;
    TrackingReceiver failingReceiver;
    failingReceiver.QueueEvent(&failedEventOne);
    failingReceiver.QueueEvent(&failedEventTwo);
    bool processingFailed = false;
    try {
        failingReceiver.DrainWithFailure();
    } catch (const std::runtime_error&) {
        processingFailed = true;
    }
    assert(processingFailed);
    assert(failedEventOne.References() == 0);
    assert(failedEventTwo.References() == 0);

    ReferenceTrackingEvent recoveryEvent;
    failingReceiver.QueueEvent(&recoveryEvent);
    failingReceiver.DrainWithoutRecording();
    assert(recoveryEvent.References() == 0);

    ReferenceTrackingEvent fanOutEvent;
    TrackingReceiver firstFanOutReceiver;
    TrackingReceiver secondFanOutReceiver;
    TestDispatcher fanOutDispatcher;
    fanOutDispatcher.RegisterReceiver(
        typeid(fanOutEvent), &firstFanOutReceiver
    );
    fanOutDispatcher.RegisterReceiver(
        typeid(fanOutEvent), &secondFanOutReceiver
    );
    fanOutDispatcher.QueueEvent(&fanOutEvent);
    fanOutDispatcher.Dispatch();
    assert(fanOutEvent.References() == 2);
    firstFanOutReceiver.DrainWithoutRecording();
    assert(fanOutEvent.References() == 1);
    secondFanOutReceiver.DrainWithoutRecording();
    assert(fanOutEvent.References() == 0);

    int liveHeapEvents = 0;
    TrackingReceiver stressReceiver;
    TestDispatcher stressDispatcher;
    HeapTrackingEvent eventTypeProbe(liveHeapEvents);
    stressDispatcher.RegisterReceiver(
        typeid(eventTypeProbe), &stressReceiver
    );
    assert(liveHeapEvents == 1);
    for (int iteration = 0; iteration < 10000; ++iteration) {
        stressDispatcher.QueueEvent(
            new HeapTrackingEvent(liveHeapEvents)
        );
        stressDispatcher.Dispatch();
        stressReceiver.DrainWithoutRecording();
        assert(liveHeapEvents == 1);
    }

    ReferenceTrackingEvent retainedEvent;
    ReferenceTrackingEvent rejectedEvent;
    TrackingReceiver boundedReceiver;
    boundedReceiver.SetMaximumPendingEventCount(1);
    boundedReceiver.SetEventQueueOverflowPolicy(
        EventQueueOverflowPolicy::RejectIncoming
    );
    boundedReceiver.QueueEvent(&retainedEvent);
    boundedReceiver.QueueEvent(&rejectedEvent);
    assert(boundedReceiver.GetPendingEventCount() == 1);
    assert(boundedReceiver.GetPeakPendingEventCount() == 1);
    assert(boundedReceiver.GetRejectedEventCount() == 1);
    assert(retainedEvent.References() == 1);
    assert(rejectedEvent.References() == 0);
    boundedReceiver.DrainWithoutRecording();
    assert(retainedEvent.References() == 0);

    ReferenceTrackingEvent oldestEvent;
    ReferenceTrackingEvent replacementEvent;
    boundedReceiver.SetEventQueueOverflowPolicy(
        EventQueueOverflowPolicy::DropOldest
    );
    boundedReceiver.QueueEvent(&oldestEvent);
    boundedReceiver.QueueEvent(&replacementEvent);
    assert(oldestEvent.References() == 0);
    assert(replacementEvent.References() == 1);
    assert(boundedReceiver.GetDroppedEventCount() == 1);
    boundedReceiver.DrainWithoutRecording();

    ReferenceTrackingEvent lowPriorityEvent;
    ReferenceTrackingEvent highPriorityEvent;
    boundedReceiver.SetEventQueueOverflowPolicy(
        EventQueueOverflowPolicy::DropLowestPriority
    );
    boundedReceiver.QueueEvent(&lowPriorityEvent, EventPriority::Low);
    boundedReceiver.QueueEvent(&highPriorityEvent, EventPriority::High);
    assert(lowPriorityEvent.References() == 0);
    assert(highPriorityEvent.References() == 1);
    boundedReceiver.DrainWithoutRecording();

    ReferenceTrackingEvent blockingEvent;
    ReferenceTrackingEvent waitingEvent;
    boundedReceiver.SetEventQueueOverflowPolicy(
        EventQueueOverflowPolicy::BlockProducer
    );
    boundedReceiver.QueueEvent(&blockingEvent);
    std::atomic<bool> producerStarted{false};
    std::atomic<bool> producerCompleted{false};
    std::thread producer([&]() {
        producerStarted.store(true);
        boundedReceiver.QueueEvent(&waitingEvent);
        producerCompleted.store(true);
    });
    while (!producerStarted.load()) {
        std::this_thread::yield();
    }
    boundedReceiver.DrainWithoutRecording();
    producer.join();
    assert(producerCompleted.load());
    assert(blockingEvent.References() == 0);
    assert(waitingEvent.References() == 1);
    boundedReceiver.DrainWithoutRecording();
    assert(waitingEvent.References() == 0);

    boundedReceiver.SetEventCollectionCapacityPolicy(
        EventCollectionCapacityPolicy::ReleaseAfterDrain
    );
    ReferenceTrackingEvent capacityEvent;
    boundedReceiver.QueueEvent(&capacityEvent);
    boundedReceiver.DrainWithoutRecording();
    assert(capacityEvent.References() == 0);
    assert(boundedReceiver.GetRetainedEventCapacity() == 0);
}
