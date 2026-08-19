#include <Arduino.h>

#include <ESPressio_EventTransport.hpp>

using namespace ESPressio;

class ExampleTransport :
    public Event::IEventTransport {

private:
    const char* _name;
    Event::IEventTransportReceiver*
        _receiver = nullptr;

public:
    explicit ExampleTransport(
        const char* name
    ) :
        _name(name) {
    }

    bool Send(
        const Event::EventTransportPacket& packet
    ) override {
        Serial.printf(
            "%s accepted message %llu (%u bytes)\n",
            _name,
            static_cast<unsigned long long>(
                packet.MessageID
            ),
            static_cast<unsigned int>(
                packet.Size
            )
        );

        return true;
    }

    void SetReceiver(
        Event::IEventTransportReceiver* receiver
    ) override {
        _receiver = receiver;
    }
};


class TelemetryEvent final :
    public Event::SerializableEvent<
        TelemetryEvent
    > {

public:
    int32_t Value = 0;

    TelemetryEvent() = default;

    explicit TelemetryEvent(
        int32_t value
    ) :
        Value(value) {
    }

    ESPRESSIO_SERIALIZABLE_TYPE(
        TelemetryEvent
    )

    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY(
            "value",
            Value
        )
    )
};


ESPRESSIO_EVENT_TRANSPORT_TYPE(
    TelemetryEvent,
    "examples.telemetry.v1"
)


ExampleTransport
    espNowTransport(
        "ESP-NOW"
    );

ExampleTransport
    udpTransport(
        "UDP"
    );


void setup() {
    Serial.begin(115200);

    auto& manager =
        Event::EventTransportManager::
            GetInstance();

    manager.RegisterTransport(
        &espNowTransport
    );

    manager.RegisterTransport(
        &udpTransport
    );

    /*
     * Default policy: TelemetryEvent is outbound through every transport.
     */
    manager.RegisterOutboundEvent<
        TelemetryEvent
    >();

    /*
     * Establish an explicit ESP-NOW override. It currently matches the
     * global policy, but remains independent from later global changes.
     */
    manager.RegisterOutboundEvent<
        TelemetryEvent
    >(
        &espNowTransport
    );

    /*
     * Disable TelemetryEvent specifically for UDP.
     *
     * Because the global default is still Outbound, the manager retains an
     * explicit None override for UDP.
     */
    Event::EventTransportUnregistrationOptions
        options;

    options.PendingOutbound =
        Event::EventTransportPendingAction::
            Discard;

    manager.UnregisterOutboundEvent<
        TelemetryEvent
    >(
        &udpTransport,
        options
    );

    manager.Initialize();

    (
        new TelemetryEvent(
            42
        )
    )->Queue();

    /*
     * The Event is now eligible for ESP-NOW only.
     */
}


void loop() {
    delay(1000);
}
