#pragma once
#include <modules/BaseModule.h>
#include "example_device_mutex.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <mutex>
#include <string>
#include <vector>

namespace Example {
// Illustrative device response, not a hardware performance model. Only the read
// task advances this state; the write task updates simulated output registers.
class ExampleDeviceModule final : public DARTWIC::Modules::BaseModule {
public:
    using BaseModule::BaseModule;
    static constexpr std::array<const char*, 18> readFields{
        "pressure_current_ma", "temperature_1", "temperature_2", "temperature_3",
        "flow", "connected", "sensor_valid", "sample_age_s",
        "igniter_command_state", "sample_counter",
        "fuel_open_coil_state", "fuel_close_coil_state",
        "oxidizer_open_coil_state", "oxidizer_close_coil_state",
        "fill_open_coil_state", "fill_close_coil_state",
        "vent_open_coil_state", "vent_close_coil_state"
    };
    static constexpr std::array<const char*, 9> writeFields{
        "fuel_open_coil", "fuel_close_coil", "oxidizer_open_coil", "oxidizer_close_coil",
        "fill_open_coil", "fill_close_coil", "vent_open_coil", "vent_close_coil", "igniter_command"
    };
    static std::vector<std::string> inputNames(const std::string& prefix, bool writer) {
        std::vector<std::string> names;
        if (writer) {
            for (auto field : writeFields) names.push_back(prefix + "_" + field);
        } else { for (auto fault : {"disconnect", "overtemperature", "sensor_bias", "missing_sensor"})
            names.push_back(prefix + "_fault_" + fault);
            names.push_back(prefix + "_run_fill");
        }
        return names;
    }
    static std::vector<std::string> outputNames(const std::string& prefix) {
        std::vector<std::string> names;
        for (auto field : readFields) names.push_back(prefix + "_" + field);
        return names;
    }
    std::array<double, 18> read(const std::array<double, 5>& inputs) {
        std::scoped_lock lock(mutex_);
        const auto now = std::chrono::steady_clock::now();
        const double dt = std::clamp(std::chrono::duration<double>(now - last_).count(), 0.0, 0.2);
        last_ = now;
        const bool connected = inputs[0] != 1;
        connected_ = connected;
        if (!connected) { coils_.fill(0); igniter_ = 0; }
        if (connected) {
            for (size_t i = 0; i < 4; ++i) {
                if (coils_[i*2] == 1 && coils_[i*2+1] == 0) positions_[i] = std::min(1.0, positions_[i] + dt*8);
                if (coils_[i*2+1] == 1 && coils_[i*2] == 0) positions_[i] = std::max(0.0, positions_[i] - dt*8);
            }
            // The periodic DCode ox-tank task owns tank physics. This device
            // reads its tank level when calculating the engine sensor response.
            const double firing = (inputs[4] > 1 && igniter_ == 1) ? std::min(positions_[0], positions_[1]) : 0;
            pressure_ += (firing*60 - pressure_) * std::min(1.0, dt*4);
            temperature_ += (300 + firing*900 - temperature_) * std::min(1.0, dt*2);
        }
        const bool valid = connected && inputs[3] != 1;
        if (valid) {
            age_ = 0;
            const double temperature = inputs[1] == 1 && igniter_ == 1 ? 1800 : temperature_;
            data_[0] = 4 + pressure_*16/100;
            data_[1] = temperature;
            data_[2] = temperature + (inputs[2] == 1 ? 350 : 1);
            data_[3] = temperature - 1;
            ++counter_;
        } else age_ += dt; // Retain the last readings and publish explicit invalidity.
        data_[4] = pressure_/6;
        data_[5] = connected ? 1 : 0; data_[6] = valid ? 1 : 0; data_[7] = age_;
        data_[8] = igniter_; data_[9] = counter_;
        // Command states report the resulting valve state, which persists after
        // a pulse ends. Open and close states are complementary during travel.
        for (size_t i=0; i<4; ++i) {
            data_[10+i*2] = positions_[i];
            data_[11+i*2] = 1.0 - positions_[i];
        }
        return data_;
    }
    void write(const std::array<double, 9>& commands) {
        std::scoped_lock lock(mutex_);
        if (!connected_) return;
        for (size_t i=0; i<4; ++i) {
            const bool open = commands[i*2] == 1, close = commands[i*2+1] == 1;
            if (open && close) {
                coils_[i*2] = coils_[i*2+1] = 0;
            } else {
                const double a = open ? 1 : 0, b = close ? 1 : 0;
                coils_[i*2] = a; coils_[i*2+1] = b;
            }
        }
        igniter_ = commands[8] == 1 ? 1 : 0;
    }
    void stopOutputs() {
        std::scoped_lock lock(mutex_);
        coils_.fill(0); igniter_ = 0;
    }
private:
    ExampleDeviceMutex mutex_;
    std::chrono::steady_clock::time_point last_ = std::chrono::steady_clock::now();
    std::array<double, 8> coils_{};
    std::array<double, 4> positions_{};
    std::array<double, 18> data_{};
    double pressure_ = 0, temperature_ = 300, igniter_ = 0, age_ = 0, counter_ = 0;
    bool connected_ = true;
};
void registerExampleDeviceDriver(DARTWIC::API::SDK_API* api);
}
