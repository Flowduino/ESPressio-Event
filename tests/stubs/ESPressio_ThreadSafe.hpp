#pragma once

#include <functional>
#include <mutex>

namespace ESPressio {
    namespace Threads {

        template <class T>
        class ReadWriteMutex {
            private:
                T _value;
                mutable std::mutex _mutex;

            public:
                explicit ReadWriteMutex(T value) : _value(value) {}

                T Get() {
                    std::lock_guard<std::mutex> lock(_mutex);
                    return _value;
                }

                void Set(T value) {
                    std::lock_guard<std::mutex> lock(_mutex);
                    _value = value;
                }

                void WithWriteLock(std::function<void(T&)> callback) {
                    std::lock_guard<std::mutex> lock(_mutex);
                    callback(_value);
                }
        };

    }
}
