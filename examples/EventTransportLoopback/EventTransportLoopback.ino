#include <Arduino.h>

#include <ESPressio_Event.hpp>
#include <ESPressio_EventListener.hpp>
#include <ESPressio_EventTransport.hpp>
#include <ESPressio_Serializable.hpp>

using namespace ESPressio;

class DistributedCounterEvent :
    public Event::Event<>,
    public Serializable::SerializableBase<DistributedCounterEvent> {
public:
    int32_t Counter = 0;

    ESPRESSIO_SERIALIZABLE_TYPE(DistributedCounterEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("counter", Counter)
    )
};

/* Stable wire identity: never use RTTI names as a protocol contract. */
ESPRESSIO_EVENT_TRANSPORT_TYPE(
    DistributedCounterEvent,
    "flowduino.example.distributed-counter.v1"
)

class LoopbackEventTransport final :
    public Event::IEventTransport {
private:
    Event::IEventTransportReceiver* _receiver = nullptr;

public:
    bool Send(
        const Event::EventTransportPacket& packet
    ) override {
        if (_receiver == nullptr) {
            return false;
        }

        /*
         * A real transport would copy/send these bytes to another device.
         * Loopback injects them into the receive path on this device so the
         * transport-independent Event machinery can be exercised alone.
         */
        _receiver->ReceiveEventTransportPacket(
            this,
            packet.Data,
            packet.Size
        );

        return true;
    }

    void SetReceiver(
        Event::IEventTransportReceiver* receiver
    ) override {
        _receiver = receiver;
    }
};

class DemoListener final :
    public Event::EventListener {
private:
    Event::EventListenerHandlePtr _handle;

public:
    DemoListener() {
        _handle = RegisterListener<DistributedCounterEvent>(
            [](
                DistributedCounterEvent* event,
                Event::EventDispatchMethod,
                Event::EventPriority
            ) {
                const auto context =
                    event->__getDispatchContext();

                Serial.printf(
                    "Counter=%ld origin=%s message=%llu\n",
                    static_cast<long>(event->Counter),
                    context.Origin == Event::EventOrigin::Remote
                        ? "remote"
                        : "local",
                    static_cast<unsigned long long>(
                        context.TransportMessageID
                    )
                );
            }
        );
    }
};

LoopbackEventTransport loopback;
DemoListener listener;

void setup() {
    Serial.begin(115200);

    auto& transports =
        Event::EventTransportManager::GetInstance();

    transports.RegisterTransport(&loopback);

    /* Equivalent individual registration is also available. */
    transports.RegisterBidirectionalEvents<
        DistributedCounterEvent
    >();

    transports.Initialize();

    auto* event = new DistributedCounterEvent();
    event->Counter = 42;
    event->Queue();
}

void loop() {
    delay(1000);
}
