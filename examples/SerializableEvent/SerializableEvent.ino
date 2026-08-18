#include <ESPressio_Event_Serializable.hpp>
#include <ESPressio_Serializable_JSON.hpp>

using namespace ESPressio;

class TemperatureEvent final :
    public Event::
        SerializableEvent<
            TemperatureEvent
        > {

    private:
        float _temperature = 0.0f;
        uint32_t _sensorId = 0;

    public:
        TemperatureEvent() = default;

        TemperatureEvent(
            uint32_t sensorId,
            float temperature
        ) :
            _temperature(temperature),
            _sensorId(sensorId) {
        }


        ESPRESSIO_SERIALIZABLE_TYPE(
            TemperatureEvent
        )

        ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(
            1
        )

        ESPRESSIO_SERIALIZABLE_PROPERTIES(
            ESPRESSIO_PROPERTY(
                "temperature",
                _temperature
            ),
            ESPRESSIO_PROPERTY(
                "sensorId",
                _sensorId
            )
        )


        float GetTemperature()
            const {
            return _temperature;
        }


        uint32_t GetSensorId()
            const {
            return _sensorId;
        }
};


void setup() {
    Serial.begin(115200);

    TemperatureEvent event(
        7,
        21.5f
    );

    /*
     * Event TimeType is a Serializable Unit.
     */
    static_assert(
        Serializable::
            IsSerializable<
                TemperatureEvent::
                    TimeType
            >
    );

    Serializable::JsonArchive
        archive;

    event.Serialize(
        archive
    );

    Serial.println(
        archive.
            ToString().
            c_str()
    );

    /*
     * Dispatch lifecycle state is not part of the serialized payload.
     * A deserialized Event is a fresh local Event and can be queued/staked
     * normally after deserialization.
     */
}


void loop() {
}
