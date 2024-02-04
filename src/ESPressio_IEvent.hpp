#pragma once

#include <ESPressio_Persistent.hpp>

using namespace ESPressio::Base;
namespace ESPressio {

    namespace Event {

        class IEvent : public Persistent<IEvent> {
            private:
                static uint16_t _classId = 0;
            public:
                static bool operator()(const MyClass& a, const MyClass& b) const {
                    return a._classID < b._classID;
                }

                static bool operator==(const MyClass& other) const {
                    return _classID == other._classID;
                }

                static void tmpSetClassID(uint16_t classId) {
                    _classId = classId;
                }
        };

    }

}