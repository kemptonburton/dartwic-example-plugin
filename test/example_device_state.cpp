#include "example_device_module.h"
#include <array>
#include <cassert>

int main() {
    Example::ExampleDeviceModule device(nlohmann::json::object(), nullptr);
    std::array<double, 5> inputs{};
    auto sensors = device.read(inputs);
    assert(sensors[5] == 1.0);

    std::array<double, 9> commands{};
    auto states = device.write(commands);
    for (double state : states) assert(state == 0.0);

    for (size_t valve = 0; valve < 4; ++valve) commands[valve * 2] = 1.0;
    commands[8] = 1.0;
    states = device.write(commands);
    for (size_t valve = 0; valve < 4; ++valve) {
        assert(states[valve * 2] == 1.0);
        assert(states[valve * 2 + 1] == 0.0);
    }
    assert(states[8] == 1.0);

    commands.fill(0.0);
    states = device.write(commands);
    for (double state : states) assert(state == 0.0);

    for (size_t valve = 0; valve < 4; ++valve) commands[valve * 2 + 1] = 1.0;
    states = device.write(commands);
    for (size_t valve = 0; valve < 4; ++valve) {
        assert(states[valve * 2] == 0.0);
        assert(states[valve * 2 + 1] == 1.0);
    }

    commands[6] = 1.0;
    states = device.write(commands);
    assert(states[6] == 0.0 && states[7] == 0.0); // Opposing vent coils are disabled.

    commands.fill(0.0);
    commands[7] = 1.0;
    states = device.write(commands);
    assert(states[7] == 1.0);
    commands[7] = 0.0;
    states = device.write(commands);
    assert(states[7] == 0.0); // vent_close_coil_state tracks vent_close_coil.

    commands[8] = 1.0;
    device.write(commands);
    inputs[0] = 1.0;
    sensors = device.read(inputs);
    assert(sensors[5] == 0.0);
    states = device.write(commands);
    for (double state : states) assert(state == 0.0);
}
