#pragma once

#include <ESPressio_Persistent.hpp>
#include <cstdint>

using namespace ESPressio::Base;
namespace ESPressio {

    namespace Event {

        class IEvent : public Persistent<IEvent> {
            protected:
                static uint16_t _classId;
            public:
                static uint16_t GetClassID();

                static void tmpSetClassID(uint16_t classId);
        };

    }

}