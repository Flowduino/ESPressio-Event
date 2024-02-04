#pragma once

#include <ESPressio_Persistent.hpp>

using namespace ESPressio::Base;
namespace ESPressio {

    namespace Event {

        class IEvent : public Persistent<IEvent> {

        };

    }

}