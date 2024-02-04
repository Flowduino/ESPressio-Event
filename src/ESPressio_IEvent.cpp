#include "ESPressio_IEvent.hpp"

using namespace ESPressio::Event;

uint16_t IEvent::_classId = 0;

uint16_t IEvent::GetClassID() {
    return _classId;
}

void IEvent::tmpSetClassID(uint16_t classId) {
    _classId = classId;
}