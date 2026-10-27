#pragma once
#include <modules/BaseModule.h>
#include "example_device_mutex.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace Example {
// Illustrative device response, not a hardware performance model. Only the read
// task advances this state; the write task updates simulated output registers.
class ExampleDeviceModule final : public DARTWIC::Modules::BaseModule {
public:
    ExampleDeviceModule(nlohmann::json cfg, DARTWIC::API::SDK_API* api)
        : BaseModule(std::move(cfg), api) {
        if (!api) return;
        const auto prefix = getParameter<std::string>("channel_prefix", "example_device");
        for (const auto& name : simulationInputNames(prefix)) api->createFixedChannel(name);
        connection_ = api->createFixedChannel(prefix + "_connected");
        disconnect_ = api->createFixedChannel(prefix + "_fault_disconnect");
        api->upsertChannelField(prefix + "_connected", DARTWIC::API::ChannelField::STALE_TIMEOUT, 3.0);
        api->upsertChannelField(prefix + "_run_fill", DARTWIC::API::ChannelField::STALE_TIMEOUT, 1.0);
    }
    static constexpr std::array<const char*, 8> readFields{
        "pressure_current_ma", "temperature_1", "temperature_2", "temperature_3",
        "flow", "sensor_valid", "sample_age_s",
        "sample_counter"
    };
    static constexpr std::array<const char*, 9> writeFields{
        "fuel_open_coil", "fuel_close_coil", "oxidizer_open_coil", "oxidizer_close_coil",
        "fill_open_coil", "fill_close_coil", "vent_open_coil", "vent_close_coil", "igniter_command"
    };
    static std::vector<std::string> simulationInputNames(const std::string& prefix) {
        return {prefix + "_fault_disconnect", prefix + "_fault_overtemperature",
            prefix + "_fault_sensor_bias", prefix + "_fault_missing_sensor", prefix + "_run_fill"};
    }
    static std::vector<std::string> outputNames(const std::string& prefix) {
        std::vector<std::string> names;
        for (auto field : readFields) names.push_back(prefix + "_" + field);
        return names;
    }
    const DARTWIC::API::FixedChannelHandle& connectionChannel() const { return connection_; }
    double monitorConnection() {
        const bool connected = !dartwic || dartwic->queryChannel(disconnect_) != 1;
        std::scoped_lock lock(mutex_);
        connected_ = connected;
        if (!connected) { coils_.fill(0); igniter_ = 0; }
        return connected ? 1.0 : 0.0;
    }
    std::array<double, 8> read(const std::array<double, 5>& inputs) {
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
        data_[5] = valid ? 1 : 0; data_[6] = age_;
        data_[7] = counter_;
        return data_;
    }
    std::array<double, 9> write(const std::array<double, 9>& commands) {
        std::scoped_lock lock(mutex_);
        if (connected_) {
            for (size_t i=0; i<4; ++i) {
                const bool open = commands[i*2] == 1, close = commands[i*2+1] == 1;
                if (open && close) {
                    coils_[i*2] = coils_[i*2+1] = 0;
                } else {
                    coils_[i*2] = open ? 1 : 0;
                    coils_[i*2+1] = close ? 1 : 0;
                }
            }
            igniter_ = commands[8] == 1 ? 1 : 0;
        }
        return outputStatesLocked();
    }
    void stopOutputs() {
        std::scoped_lock lock(mutex_);
        coils_.fill(0); igniter_ = 0;
    }
private:
    std::array<double, 9> outputStatesLocked() const {
        std::array<double, 9> result{};
        std::copy(coils_.begin(), coils_.end(), result.begin());
        result[8] = igniter_;
        return result;
    }
    DARTWIC::API::FixedChannelHandle connection_, disconnect_;
    ExampleDeviceMutex mutex_;
    std::chrono::steady_clock::time_point last_ = std::chrono::steady_clock::now();
    std::array<double, 8> coils_{};
    std::array<double, 4> positions_{};
    std::array<double, 8> data_{};
    double pressure_ = 0, temperature_ = 300, igniter_ = 0, age_ = 0, counter_ = 0;
    bool connected_ = true;
};
void registerExampleDeviceDriver(DARTWIC::API::SDK_API* api);
}
