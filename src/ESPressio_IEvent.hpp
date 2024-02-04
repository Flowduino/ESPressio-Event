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
        };

    }

}