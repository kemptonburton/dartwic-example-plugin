#include "rocket_sim_module.h"
#include <sdk/notifications/NotificationMuteHandle.h>
#include <chrono>
#include <memory>

namespace Example {
namespace {
// The simulated endpoint replaces a network scan. The provisioning contract
// is the same module-discovery flow used by the Modbus plugin.
class RocketDiscovery {
public:
    explicit RocketDiscovery(DARTWIC::API::SDK_API* api)
        : api_(api), mute_("rocket-device-discovery", [this]{ withdraw(); }, [this]{ request_id_.clear(); }) {}
    void tick() {
        const bool present = api_->queryChannelField("rocket_sim_fault_disconnect", DARTWIC::API::ChannelField::VALUE, 0.0) != 1;
        if (!announced_ || present != present_) {
            api_->writeLog("Rocket discovery", present ? "Found simulated rocket device" : "Simulated rocket device disappeared", "stdout", present ? "info" : "warning");
            present_ = present; announced_ = true;
        }
        if (!present || api_->getModuleInstance("rocket_device") || mute_.refresh(*api_)) { withdraw(); return; }
        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        const auto request = api_->requestInterfaceUi("dartwic.module-discovery", {
            {"discovery_id", "mock-rocket-device"}, {"device_type", "rocket_sim"},
            {"display_name", "Mock Rocket Device"}, {"endpoint", {{"host", "simulated"}, {"port", 1}}},
            {"presence", {{"available", true}, {"valid_until_unix_ms", now + 3000}}},
            {"channels", nlohmann::json::array({
                {{"name", "rocket_sim_pressure_current_ma"}, {"direction", "read"}, {"units", "mA"}},
                {{"name", "rocket_sim_igniter_command"}, {"direction", "write"}}
            })},
            {"provisioning", {{"module_type", "rocket_sim"}, {"suggested_instance_name", "rocket_device"},
                {"tasks", nlohmann::json::array({
                    {{"name_suffix", "_read"}, {"task_type", "rocket_read"}, {"arguments", {{"channel_prefix", "rocket_sim"}}}},
                    {{"name_suffix", "_write"}, {"task_type", "rocket_write"}, {"arguments", {{"channel_prefix", "rocket_sim"}}}}
                })}}}
        }, options(true));
        request_id_ = request.value("request_id", "");
    }
private:
    nlohmann::json options(bool reopen) const {
        return {{"request_key", "mock-rocket-device"}, {"merge_key", "module-discovery"},
            {"notification_id", "rocket-device-discovery"}, {"silenceable", true},
            {"mute_scope", "engine"}, {"reopen_completed", reopen}};
    }
    void withdraw() {
        if (request_id_.empty()) return;
        const auto request = api_->getInterfaceUiRequest(request_id_);
        if (request.value("status", "") == "pending") {
            auto payload = request.at("payload");
            payload["presence"] = {{"available", false}, {"valid_until_unix_ms", 0}};
            api_->requestInterfaceUi("dartwic.module-discovery", std::move(payload), options(false));
        }
        request_id_.clear();
    }
    DARTWIC::API::SDK_API* api_;
    DARTWIC::API::NotificationMuteHandle mute_;
    std::string request_id_;
    bool announced_ = false, present_ = false;
};
}
void registerRocketDiscovery(DARTWIC::API::SDK_API* api) {
    auto discovery = std::make_shared<RocketDiscovery>(api);
    api->registerLoop("rocket_device_discovery", "Mock Rocket Device Discovery", {
        .on_loop = [discovery]{ discovery->tick(); }, .target_frequency_hz = 1.0
    });
}
}
