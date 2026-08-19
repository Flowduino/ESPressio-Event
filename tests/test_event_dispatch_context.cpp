#include <cassert>
#include <type_traits>

#include "../src/ESPressio_EventTransportTypes.hpp"

int main() {
    using namespace ESPressio::Event;

    static_assert(
        std::is_same<
            decltype(
                EventDispatchContext{} ==
                EventDispatchContext{}
            ),
            bool
        >::value,
        "EventDispatchContext must be equality comparable."
    );

    constexpr EventDispatchContext localA{};
    constexpr EventDispatchContext localB{};

    static_assert(
        localA == localB,
        "Equivalent default contexts must compare equal."
    );

    constexpr EventDispatchContext remoteA{
        EventOrigin::Remote,
        42,
        3
    };

    constexpr EventDispatchContext remoteB{
        EventOrigin::Remote,
        42,
        3
    };

    constexpr EventDispatchContext differentOrigin{
        EventOrigin::Local,
        42,
        3
    };

    constexpr EventDispatchContext differentMessage{
        EventOrigin::Remote,
        43,
        3
    };

    constexpr EventDispatchContext differentHop{
        EventOrigin::Remote,
        42,
        4
    };

    static_assert(remoteA == remoteB);
    static_assert(remoteA != differentOrigin);
    static_assert(remoteA != differentMessage);
    static_assert(remoteA != differentHop);

    assert(remoteA == remoteB);
    assert(!(remoteA != remoteB));

    return 0;
}
