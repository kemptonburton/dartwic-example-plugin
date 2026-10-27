#include "example_device_module.h"
#include <sdk/notifications/NotificationMuteHandle.h>
#include <sdk/discovery/DiscoveryResources.h>
#include <algorithm>
#include <chrono>
#include <memory>

namespace Example {
namespace {
// The simulated endpoint replaces a network scan. The provisioning contract
// is the same module-discovery flow used by the Modbus plugin.
class ExampleDeviceDiscovery {
public:
    explicit ExampleDeviceDiscovery(DARTWIC::API::SDK_API* api)
        : api_(api), mute_("example-device-discovery", [this]{ withdraw(); }, [this]{ request_id_.clear(); }) {}
    void tick() {
        const bool present = true; // The example endpoint is always discoverable until configured.
        if (!announced_ || present != present_) {
            api_->writeLog("Example device discovery", present ? "Found simulated example device" : "Simulated example device disappeared", "stdout", present ? "info" : "warning");
            present_ = present; announced_ = true;
        }
        const auto modules = api_->getModuleInstances("example_device_plugin");
        const bool configured = std::any_of(modules.begin(), modules.end(), [](const auto& module) {
            return module.module_type_id == "example_device";
        });
        if (!present || configured || mute_.refresh(*api_)) { withdraw(); return; }
        nlohmann::json read_mappings = nlohmann::json::array();
        nlohmann::json write_mappings = nlohmann::json::array();
        for (const auto* field : ExampleDeviceModule::readFields)
            read_mappings.push_back({{"device_field", field}, {"channel", std::string("example_device_") + field}});
        for (const auto* field : ExampleDeviceModule::writeFields)
            write_mappings.push_back({{"device_field", field}, {"channel", std::string("example_device_") + field}});
        nlohmann::json entries = nlohmann::json::array(), channels = nlohmann::json::array();
        const auto append = [&](const char* field, bool writer) {
            const std::string name = std::string("example_device_") + field;
            std::string label = field;
            std::replace(label.begin(), label.end(), '_', ' ');
            const nlohmann::json channel = {{"direction", writer ? "write" : "read"}};
            entries.push_back({{"id", std::string(writer ? "write_" : "read_") + field}, {"label", label},
                {"task_id", writer ? "write" : "read"}, {"channel_name", name}, {"channel", channel},
                {"mapping", {{"device_field", field}, {"direction", writer ? "channel_to_device" : "device_to_channel"}}}});
            auto advertised = channel; advertised["name"] = name; channels.push_back(std::move(advertised));
        };
        for (auto field : ExampleDeviceModule::readFields) append(field, false);
        for (auto field : ExampleDeviceModule::writeFields) append(field, true);
        DARTWIC::API::DiscoveryResources module_resources, write_resources;
        for (const auto& name : ExampleDeviceModule::simulationInputNames("example_device"))
            module_resources.channels.push_back({name, "Simulation input", "input"});
        module_resources.channels.push_back({"example_device_connected", "Connection status", "output"});
        for (const auto* field : ExampleDeviceModule::writeFields)
            write_resources.channels.push_back({std::string("example_device_") + field + "_state", "Applied output state", "output"});
        const auto editor_tasks = nlohmann::json::array({
            {{"id", "read"}, {"label", "Read"}, {"name_suffix", "_read"}, {"task_type", "example_read"}, {"argument_key", "read_mappings"}},
            {{"id", "write"}, {"label", "Write"}, {"name_suffix", "_write"}, {"task_type", "example_write"}, {"argument_key", "write_mappings"}, {"resources", write_resources.toJson()}}
        });
        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        const auto request = api_->requestInterfaceUi("dartwic.module-discovery", {
            {"discovery_id", "example-device-1"}, {"device_type", "example_device"},
            {"display_name", "Example Device"}, {"endpoint", {{"host", "simulated"}, {"port", 1}}},
            {"presence", {{"available", true}, {"valid_until_unix_ms", now + 3000}}},
            {"metadata", {{"protocol", "Simulated"}, {"endpoint_label", "Simulated device 1"},
                {"module_map_summary", "Simulated device with suggested read and write tasks."}}},
            {"channels", channels},
            {"provisioning", {{"module_type", "example_device"}, {"suggested_instance_name", "example_device_1"}, {"resources", module_resources.toJson()},
                {"channel_suggestion_editor", {{"kind", "explicit_mappings"}, {"tasks", editor_tasks}, {"entries", entries}}},
                {"tasks", nlohmann::json::array({
                    {{"name_suffix", "_read"}, {"task_type", "example_read"}, {"arguments", {{"channel_prefix", "example_device"}, {"read_mappings", read_mappings}}}},
                    {{"name_suffix", "_write"}, {"task_type", "example_write"}, {"arguments", {{"channel_prefix", "example_device"}, {"write_mappings", write_mappings}}}}
                })}}}
        }, options(true));
        request_id_ = request.value("request_id", "");
    }
private:
    nlohmann::json options(bool reopen) const {
        return {{"request_key", "example-device-1"}, {"merge_key", "module-discovery"},
            {"notification_id", "example-device-discovery"}, {"silenceable", true},
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
void registerExampleDeviceDiscovery(DARTWIC::API::SDK_API* api) {
    auto discovery = std::make_shared<ExampleDeviceDiscovery>(api);
    api->registerLoop("example_device_1_discovery", "Example Device Discovery", {
        .on_loop = [discovery]{ discovery->tick(); }, .target_frequency_hz = 1.0
    });
}
}
