#pragma once
#include <modules/BaseModule.h>
#include "example_device_mutex.h"
#include <array>
#include <stdexcept>

namespace Example {
// Deterministic local registers, not a physical device timing model.
class StressDeviceModule final : public DARTWIC::Modules::BaseModule {
public:
    StressDeviceModule(nlohmann::json cfg, DARTWIC::API::SDK_API* api)
        : BaseModule(std::move(cfg), api), maximum_(getParameter<int>("maximum", 1000)), threshold_(getParameter<int>("threshold", 800)) {
        if (api) {
            const auto name = getConfig<std::string>("name", "stress_device") + ".info.connected";
            connection_ = api->createFixedChannel(name);
            api->upsertChannelField(name, DARTWIC::API::ChannelField::STALE_TIMEOUT, 3.0);
        }
        if (maximum_ < 3 || threshold_ <= 1 || threshold_ >= maximum_) throw std::invalid_argument("Invalid stress waveform bounds.");
    }
    const DARTWIC::API::FixedChannelHandle& connectionChannel() const { return connection_; }
    double read();
    void write(const std::array<double, 5>& values);
    nlohmann::json status();
private:
    DARTWIC::API::FixedChannelHandle connection_;
    ExampleDeviceMutex mutex_;
    const int maximum_, threshold_;
    int value_ = 0, direction_ = 1, latched_ = 0;
    uint64_t reads_ = 0, writes_ = 0, mismatches_ = 0;
    std::array<double, 5> registers_{};
    nlohmann::json examples_ = nlohmann::json::array();
};
void registerStressDevice(DARTWIC::API::SDK_API* api);
}
