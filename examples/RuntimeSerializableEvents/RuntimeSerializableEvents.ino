#include <Arduino.h>
#include <ESPressio_Event.hpp>
#include <ESPressio_EventTransport.hpp>
#include <ESPressio_Serializable.hpp>

using namespace ESPressio;

class OperatorCommandEvent :
    public Event::Event<>,
    public Serializable::SerializableBase<OperatorCommandEvent> {
public:
    int32_t Value = 0;

    ESPRESSIO_SERIALIZABLE_TYPE(OperatorCommandEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("value", Value)
    )
};

ESPRESSIO_EVENT_TRANSPORT_TYPE(
    OperatorCommandEvent,
    "flowduino.example.operator-command.v1"
)

void setup() {
    ::Serial.begin(115200);

    auto& manager = Event::EventTransportManager::GetInstance();
    manager.RegisterBidirectionalEvent<OperatorCommandEvent>();

    // Runtime discovery requires no compile-time knowledge of the Event type.
    for (const auto& descriptor : manager.GetRegisteredSerializableEvents()) {
        ::Serial.print("Registered: ");
        ::Serial.println(descriptor.TypeName.c_str());
        ::Serial.print("Schema: ");
        ::Serial.println(descriptor.SchemaVersion);
        for (const auto& property : descriptor.Properties) {
            ::Serial.print("  ");
            ::Serial.print(property.Name.c_str());
            ::Serial.print(" : ");
            ::Serial.println(property.Type.c_str());
        }
    }

    // A future Serial/REST/WebSocket console can obtain this node from JSON.
    // Event itself remains independent of ArduinoJson.
    Serializable::SerializationNode payload(
        Serializable::SerializationNodeType::Object
    );
    payload.Set("__schemaVersion", Serializable::Detail::ToNode(uint32_t{1}));
    payload.Set("value", Serializable::Detail::ToNode(int32_t{42}));

    auto result = manager.CreateSerializableEvent(
        "flowduino.example.operator-command.v1",
        payload
    );

    if (!result) {
        ::Serial.println("Runtime Event construction failed.");
        for (const auto& issue : result.Deserialization.Issues()) {
            ::Serial.print(issue.Path.c_str());
            ::Serial.print(": ");
            ::Serial.println(issue.Message.c_str());
        }
        return;
    }

    Event::EventTransportManager::DispatchSerializableEvent(
        std::move(result.Event),
        Event::EventDispatchMethod::Queue,
        Event::EventPriority::Normal
    );

    ::Serial.println("Runtime Event dispatched.");
}

void loop() {}
