#include <cassert>
#include <stdexcept>
#include <type_traits>

#include "ESPressio_EventListener.hpp"

using namespace ESPressio::Event;

class TestEvent final : public IEvent {
    private:
        int _references = 1;
        EventTime _age;

    public:
        explicit TestEvent(unsigned long ageMilliseconds = 0)
            : _age(ageMilliseconds, ESPressio::Units::Milli) {}
        void __ref() override { ++_references; }
        void __unref() override { --_references; }
        void __dispatch() override {}
        void Queue(EventPriority = EventPriority::Normal) override {}
        void Stack(EventPriority = EventPriority::Normal) override {}
        EventTime GetDispatchTime() override { return EventTime(0); }
        EventTime GetTimeSinceDispatch() override { return _age; }
        int References() const { return _references; }
};

class OtherEvent final : public IEvent {
    public:
        void __ref() override {}
        void __unref() override {}
        void __dispatch() override {}
        void Queue(EventPriority = EventPriority::Normal) override {}
        void Stack(EventPriority = EventPriority::Normal) override {}
        EventTime GetDispatchTime() override { return EventTime(0); }
        EventTime GetTimeSinceDispatch() override { return EventTime(0); }
};

class TestObserver final : public IEventObserver<TestEvent> {
    public:
        int calls = 0;
        bool interested = true;
        TestEvent* lastEvent = nullptr;
        EventDispatchMethod lastMethod = EventDispatchMethod::Queue;
        EventPriority lastPriority = EventPriority::Normal;
        IEventListenerHandle* unregisterOnEvent = nullptr;

        void OnEvent(
            TestEvent* event,
            EventDispatchMethod dispatchMethod,
            EventPriority priority) override {
            ++calls;
            lastEvent = event;
            lastMethod = dispatchMethod;
            lastPriority = priority;
            if (unregisterOnEvent != nullptr) {
                unregisterOnEvent->Unregister();
            }
        }

        bool IsInterestedInEvent(TestEvent*) override {
            return interested;
        }
};

class MultiEventObserver final :
    public IEventObserver<TestEvent>,
    public IEventObserver<OtherEvent> {
    public:
        int testCalls = 0;
        int otherCalls = 0;

        void OnEvent(TestEvent*, EventDispatchMethod, EventPriority) override {
            ++testCalls;
        }

        void OnEvent(OtherEvent*, EventDispatchMethod, EventPriority) override {
            ++otherCalls;
        }
};

class TrackingEventListener final : public EventListener {
    public:
        int registrations = 0;
        int unregistrations = 0;

        void Shutdown() { UnregisterAllListeners(); }

    protected:
        void OnListenerRegistered(std::type_index) override { ++registrations; }
        void OnListenerUnregistered(std::type_index) override { ++unregistrations; }
};

static_assert(std::is_base_of<
    ESPressio::Observable::IObserver,
    IEventObserver<TestEvent>>::value,
    "Event Observers must satisfy the ESPressio Observable IObserver interface");
static_assert(std::is_convertible<MultiEventObserver*,
    ESPressio::Observable::IObserver*>::value,
    "Multi-Event Observers must have one unambiguous IObserver identity");

void Process(
    EventListener& listener,
    IEvent& event,
    EventDispatchMethod method = EventDispatchMethod::Queue,
    EventPriority priority = EventPriority::Normal) {
    listener.ProcessEvent(&event, method, priority);
}

int main() {
    TrackingEventListener listener;
    TestObserver observer;

    bool nullThrown = false;
    try { listener.RegisterObserver<TestEvent>(nullptr); }
    catch (const ESPressio::Observable::InvalidObserverRegistrationException&) {
        nullThrown = true;
    }
    assert(nullThrown);

    MultiEventObserver multiObserver;
    IEventListenerHandle* multiTestHandle =
        listener.RegisterObserver<TestEvent>(&multiObserver);
    IEventListenerHandle* multiOtherHandle =
        listener.RegisterObserver<OtherEvent>(&multiObserver);
    TestEvent multiTestEvent;
    OtherEvent multiOtherEvent;
    Process(listener, multiTestEvent);
    Process(listener, multiOtherEvent);
    assert(multiObserver.testCalls == 1);
    assert(multiObserver.otherCalls == 1);
    delete multiTestHandle;
    delete multiOtherHandle;

    IEventListenerHandle* observerHandle =
        listener.RegisterObserver<TestEvent>(&observer);
    assert(observerHandle->IsRegistered());
    const int routingRegistrations = listener.registrations;
    const int routingUnregistrations = listener.unregistrations;

    int callbackCalls = 0;
    IEventListenerHandle* callbackHandle = listener.RegisterListener<TestEvent>(
        [&](TestEvent*, EventDispatchMethod, EventPriority) { ++callbackCalls; });
    assert(listener.registrations == routingRegistrations);

    TestEvent event;
    Process(listener, event, EventDispatchMethod::Stack, EventPriority::High);
    assert(observer.calls == 1);
    assert(observer.lastEvent == &event);
    assert(observer.lastMethod == EventDispatchMethod::Stack);
    assert(observer.lastPriority == EventPriority::High);
    assert(callbackCalls == 1);
    assert(event.References() == 1);

    observerHandle->Unregister();
    observerHandle->Unregister();
    assert(!observerHandle->IsRegistered());
    Process(listener, event);
    assert(observer.calls == 1);
    assert(callbackCalls == 2);
    assert(listener.unregistrations == routingUnregistrations);
    delete observerHandle;
    delete callbackHandle;
    assert(listener.unregistrations == routingUnregistrations + 1);

    TestObserver customObserver;
    IEventListenerHandle* customHandle = listener.RegisterObserver<TestEvent>(
        &customObserver, EventListenerInterest::Custom);
    customObserver.interested = false;
    Process(listener, event);
    assert(customObserver.calls == 0);
    customObserver.interested = true;
    Process(listener, event);
    assert(customObserver.calls == 1);
    delete customHandle;

    TestObserver youngObserver;
    IEventListenerHandle* youngHandle = listener.RegisterObserver<TestEvent>(
        &youngObserver,
        EventListenerInterest::YoungerThan,
        EventTime(10, ESPressio::Units::Milli)
    );
    TestEvent youngEvent(9);
    TestEvent oldEvent(10);
    Process(listener, youngEvent);
    Process(listener, oldEvent);
    assert(youngObserver.calls == 1);
    delete youngHandle;

    TestObserver selfRemovingObserver;
    IEventListenerHandle* selfRemovingHandle =
        listener.RegisterObserver<TestEvent>(&selfRemovingObserver);
    selfRemovingObserver.unregisterOnEvent = selfRemovingHandle;
    Process(listener, event);
    Process(listener, event);
    assert(selfRemovingObserver.calls == 1);
    assert(!selfRemovingHandle->IsRegistered());
    delete selfRemovingHandle;

    IEventListenerHandle* throwingHandle = listener.RegisterListener<TestEvent>(
        [](TestEvent*, EventDispatchMethod, EventPriority) {
            throw std::runtime_error("expected callback failure");
        });
    bool callbackThrown = false;
    const int referencesBeforeThrow = event.References();
    try { Process(listener, event); }
    catch (const std::runtime_error&) { callbackThrown = true; }
    assert(callbackThrown);
    assert(event.References() == referencesBeforeThrow);
    delete throwingHandle;

    OtherEvent otherEvent;
    Process(listener, otherEvent);

    TestObserver survivingObserver;
    IEventListenerHandle* survivingHandle = nullptr;
    {
        EventListener temporaryListener;
        survivingHandle = temporaryListener.RegisterObserver<TestEvent>(
            &survivingObserver);
        assert(survivingHandle->IsRegistered());
    }
    assert(!survivingHandle->IsRegistered());
    survivingHandle->Unregister();
    delete survivingHandle;

    TrackingEventListener shutdownListener;
    IEventListenerHandle* shutdownHandle =
        shutdownListener.RegisterObserver<TestEvent>(&survivingObserver);
    shutdownListener.Shutdown();
    assert(shutdownListener.unregistrations == 1);
    assert(!shutdownHandle->IsRegistered());
    delete shutdownHandle;
}
