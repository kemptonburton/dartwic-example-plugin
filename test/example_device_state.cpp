#include "example_device_module.h"
#include <array>
#include <cassert>
#include <chrono>
#include <thread>

int main() {
    Example::ExampleDeviceModule device(nlohmann::json::object(), nullptr);
    std::array<double, 5> inputs{};
    auto values = device.read(inputs);
    for (size_t valve = 0; valve < 4; ++valve) {
        assert(values[10 + valve * 2] == 0.0);
        assert(values[11 + valve * 2] == 1.0);
    }

    std::array<double, 9> commands{};
    for (size_t valve = 0; valve < 4; ++valve) commands[valve * 2] = 1.0;
    commands[8] = 1.0;
    device.write(commands);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    values = device.read(inputs);
    assert(values[8] == 1.0);
    for (size_t valve = 0; valve < 4; ++valve) {
        assert(values[10 + valve * 2] == 1.0);
        assert(values[11 + valve * 2] == 0.0);
    }

    commands.fill(0.0);
    device.write(commands);
    values = device.read(inputs);
    for (size_t valve = 0; valve < 4; ++valve) assert(values[10 + valve * 2] == 1.0);

    for (size_t valve = 0; valve < 4; ++valve) commands[valve * 2 + 1] = 1.0;
    device.write(commands);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    values = device.read(inputs);
    for (size_t valve = 0; valve < 4; ++valve) {
        assert(values[10 + valve * 2] == 0.0);
        assert(values[11 + valve * 2] == 1.0);
    }

    inputs[0] = 1.0;
    values = device.read(inputs);
    assert(values[5] == 0.0);
    assert(values[6] == 0.0);
    assert(values[8] == 0.0);
}
