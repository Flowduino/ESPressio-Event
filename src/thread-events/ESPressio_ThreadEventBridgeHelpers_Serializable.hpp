#pragma once

#include <exception>
#include <string>

namespace ESPressio {
namespace Event {
namespace Internal {

inline std::string DescribeThreadBridgeException(
    std::exception_ptr cause
) {
    if (!cause) {
        return std::string();
    }

    try {
        std::rethrow_exception(cause);
    } catch (const std::exception& exception) {
        return exception.what();
    } catch (...) {
        return "Unknown exception";
    }
}

}
}
}
