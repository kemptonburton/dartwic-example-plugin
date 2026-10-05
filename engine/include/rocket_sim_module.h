#pragma once
#include <modules/BaseModule.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <mutex>

namespace Example {
// Illustrative device response, not a rocket performance model. Only the read
// task advances this state; the write task updates simulated output registers.
class RocketSimModule final : public DARTWIC::Modules::BaseModule {
public:
    using BaseModule::BaseModule;
    static constexpr std::array<const char*, 4> valves{"fuel", "oxidizer", "fill", "vent"};
    static constexpr std::array<const char*, 24> readFields{
        "pressure_current_ma", "temperature_1", "temperature_2", "temperature_3",
        "run_fill", "supply_fill", "flow", "connected", "sensor_valid", "sample_age_s",
        "igniter_applied", "sample_counter", "fuel_position", "oxidizer_position",
        "fill_position", "vent_position", "fuel_open_applied", "fuel_close_applied",
        "oxidizer_open_applied", "oxidizer_close_applied", "fill_open_applied",
        "fill_close_applied", "vent_open_applied", "vent_close_applied"
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
            names.push_back(prefix + "_phase");
        }
        return names;
    }
    static std::vector<std::string> outputNames(const std::string& prefix) {
        std::vector<std::string> names;
        for (auto field : readFields) names.push_back(prefix + "_" + field);
        return names;
    }
    std::array<double, 24> read(const std::array<double, 5>& faults) {
        std::scoped_lock lock(mutex_);
        const auto now = std::chrono::steady_clock::now();
        const double dt = std::clamp(std::chrono::duration<double>(now - last_).count(), 0.0, 0.2);
        last_ = now;
        for (size_t index = 0; index < 4; ++index) {
            if (faults[index] != last_faults_[index]) {
                static constexpr std::array<const char*, 4> labels{"disconnect", "overtemperature", "sensor disagreement", "missing sensor"};
                dartwic->writeLog("Rocket device", std::string(labels[index]) + (faults[index] == 1 ? ": fault injected" : ": recovered"));
                last_faults_[index] = faults[index];
            }
        }
        const bool connected = faults[0] != 1;
        if (!announced_ || connected != connected_) {
            dartwic->writeLog("Rocket device", connected ? "Simulated device connected" : "Simulated device disconnected; outputs de-energized",
                "stdout", connected ? "info" : "warning");
            announced_ = true;
        }
        connected_ = connected;
        if (!connected) { coils_.fill(0); igniter_ = 0; }
        if (connected) {
            for (size_t i = 0; i < 4; ++i) {
                if (coils_[i*2] == 1 && coils_[i*2+1] == 0) positions_[i] = std::min(1.0, positions_[i] + dt*8);
                if (coils_[i*2+1] == 1 && coils_[i*2] == 0) positions_[i] = std::max(0.0, positions_[i] - dt*8);
            }
            // Supply is three times the run-tank capacity. Replenish it in idle so
            // the same example can be repeated without another operator control.
            const double transfer = std::min({positions_[2]*15*dt, supply_*3, 100-run_});
            supply_ -= transfer/3;
            run_ = std::clamp(run_ + transfer - (positions_[3]*12 + positions_[0]*positions_[1]*2)*dt, 0.0, 100.0);
            if (faults[4] == 0) supply_ = std::min(100.0, supply_ + dt*3);
            const double firing = (run_ > 1 && igniter_ == 1) ? std::min(positions_[0], positions_[1]) : 0;
            pressure_ += (firing*60 - pressure_) * std::min(1.0, dt*4);
            temperature_ += (300 + firing*900 - temperature_) * std::min(1.0, dt*2);
        }
        const bool valid = connected && faults[3] != 1;
        if (valid) {
            age_ = 0;
            const double temperature = faults[1] == 1 && faults[4] >= 2 ? 1800 : temperature_;
            data_[0] = 4 + pressure_*16/100;
            data_[1] = temperature;
            data_[2] = temperature + (faults[2] == 1 ? 350 : 1);
            data_[3] = temperature - 1;
            ++counter_;
        } else age_ += dt; // Retain the last readings and publish explicit invalidity.
        data_[4] = run_; data_[5] = supply_;
        data_[6] = pressure_/6;
        data_[7] = connected ? 1 : 0; data_[8] = valid ? 1 : 0; data_[9] = age_;
        data_[10] = igniter_; data_[11] = counter_;
        for (size_t i=0; i<4; ++i) data_[12+i] = positions_[i];
        for (size_t i=0; i<8; ++i) data_[16+i] = coils_[i];
        return data_;
    }
    void write(const std::array<double, 9>& commands) {
        std::scoped_lock lock(mutex_);
        if (!connected_) return;
        for (size_t i=0; i<4; ++i) {
            const bool open = commands[i*2] == 1, close = commands[i*2+1] == 1;
            if (open && close) {
                if (!conflicts_[i]) dartwic->writeLog("Rocket device", std::string(valves[i]) + ": rejected simultaneous open/close coils", "stderr", "error");
                conflicts_[i] = true;
                coils_[i*2] = coils_[i*2+1] = 0;
            } else {
                conflicts_[i] = false;
                const double a = open ? 1 : 0, b = close ? 1 : 0;
                if (coils_[i*2] != a || coils_[i*2+1] != b)
                    dartwic->writeLog("Rocket device", std::string(valves[i]) + (a ? ": open pulse applied" : b ? ": close pulse applied" : ": coils off"));
                coils_[i*2] = a; coils_[i*2+1] = b;
            }
        }
        const double next = commands[8] == 1 ? 1 : 0;
        if (next != igniter_) dartwic->writeLog("Rocket device", next == 1 ? "Igniter applied" : "Igniter off");
        igniter_ = next;
    }
    void stopOutputs() {
        std::scoped_lock lock(mutex_);
        coils_.fill(0); igniter_ = 0;
        dartwic->writeLog("Rocket device", "Write task stopped; simulated coils and igniter off");
    }
private:
    std::mutex mutex_;
    std::chrono::steady_clock::time_point last_ = std::chrono::steady_clock::now();
    std::array<double, 8> coils_{};
    std::array<double, 4> positions_{};
    std::array<bool, 4> conflicts_{};
    std::array<double, 24> data_{};
    std::array<double, 4> last_faults_{};
    double run_ = 0, supply_ = 100, pressure_ = 0, temperature_ = 300, igniter_ = 0, age_ = 0, counter_ = 0;
    bool connected_ = true, announced_ = false;
};
void registerRocketDriver(DARTWIC::API::SDK_API* api);
}
