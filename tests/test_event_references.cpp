#include <cassert>
#include <vector>

#include "ESPressio_EventDispatcher.hpp"

using namespace ESPressio::Event;

class ReferenceTrackingEvent final : public IEvent {
    private:
        int _references = 0;

    public:
        void __ref() override { ++_references; }
        void __unref() override {
            assert(_references > 0);
            --_references;
        }
        void __dispatch() override { }
        void Queue(EventPriority = EventPriority::Normal) override { }
        void Stack(EventPriority = EventPriority::Normal) override { }
        EventTime GetDispatchTime() override { return EventTime(0); }
        EventTime GetTimeSinceDispatch() override { return EventTime(0); }
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
                event->__unref();
            });
        }
};

class TestDispatcher final : public EventDispatcher {
    public:
        void Dispatch() { DispatchEvents(); }
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
}
