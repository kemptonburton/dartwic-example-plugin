#include "example_device_plugin.h"
#include "example_transport.h"
#include "rocket_sim_module.h"
#include <tempest/Peer.h>
#include <sdk/notifications/NotificationMuteHandle.h>
#include <array>
#include <algorithm>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
// A simulated acquisition/write pair for the fixed-channel driver guide.
// No device or networking guarantee is implied by the applied-value echo.
struct FixedDemoState {
    DARTWIC::API::FixedChannelHandle command;
    std::array<DARTWIC::API::FixedChannelHandle, 2> outputs;
    std::array<double, 1> commands{};
    std::array<double, 2> values{};
    double ticks = 0;
};

struct MockDriverTaskState {
    std::shared_ptr<Example::ExampleDeviceModule> module;
    DARTWIC::API::FixedChannelHandle command_handle;
    std::array<DARTWIC::API::FixedChannelHandle, 3> outputs;
    std::array<double, 1> command{};
    std::array<double, 3> values{};
};

struct MockChannelBindings {
    std::string measurement;
    std::string command;
    std::string applied;
    std::string connected;
};

std::shared_ptr<Example::ExampleDeviceModule> mockModule(
    DARTWIC::API::SDK_API* api,
    DARTWIC::API::TaskRuntime& runtime
) {
    const std::string instance = runtime.getArguments().value("module_instance_name", std::string{});
    if (instance.empty()) throw std::runtime_error("Mock driver tasks require module_instance_name.");
    auto module = std::dynamic_pointer_cast<Example::ExampleDeviceModule>(api->getModuleInstance(instance));
    if (!module || module->dartwic == nullptr) {
        throw std::runtime_error("Configured mock device module `" + instance + "` is not available.");
    }
    return module;
}

std::string mockPrefix(DARTWIC::API::TaskRuntime& runtime) {
    return runtime.getArguments().value("channel_prefix", std::string{"mock_device"});
}

std::string mockChannel(DARTWIC::API::TaskRuntime& runtime,
                        const char* argument,
                        const std::string& fallback) {
    const auto channel = runtime.getArguments().value(argument, fallback);
    if (channel.empty()) throw std::runtime_error(std::string{"Mock driver requires `"} + argument + "`.");
    return channel;
}

MockChannelBindings mockChannels(DARTWIC::API::TaskRuntime& runtime) {
    const auto prefix = mockPrefix(runtime);
    return {
        .measurement = mockChannel(runtime, "measurement_channel", prefix + "_measurement"),
        .command = mockChannel(runtime, "command_channel", prefix + "_command"),
        .applied = mockChannel(runtime, "applied_channel", prefix + "_applied"),
        .connected = mockChannel(runtime, "connected_channel", prefix + "_connected"),
    };
}

void configureMockDriverTask(
    DARTWIC::API::SDK_API* api,
    DARTWIC::API::TaskRuntime& runtime,
    const bool is_write
) {
    using namespace DARTWIC::API;
    auto module = mockModule(api, runtime);
    const auto channels = mockChannels(runtime);
    const auto task_channels = is_write
        ? std::vector<std::string>{channels.command}
        : std::vector<std::string>{channels.measurement, channels.applied, channels.connected};
    auto state = std::make_shared<MockDriverTaskState>();
    state->module = module;
    for (size_t index = 0; index < task_channels.size(); ++index) {
        const auto& channel = task_channels[index];
        const auto handle = module->dartwic->createFixedChannel(channel, 0.0);
        if (is_write) state->command_handle = handle;
        else state->outputs[index] = handle;
        module->dartwic->upsertChannelField(channel, ChannelField::RECORD_MODE, RecordMode::Never, ChannelStorage::Fixed);
    }
    runtime.setFixedInputChannels(is_write ? std::vector<std::string>{channels.command} : std::vector<std::string>{});
    runtime.setTypedRuntimeContext(is_write ? "mock-driver-write" : "mock-driver-read", state);
}

class MockDeviceDiscovery {
public:
    explicit MockDeviceDiscovery(DARTWIC::API::SDK_API* api)
        : api_(api), mute_handle_("device-discovery:mock-device-1",
            [this] { offered_ = false; },
            [this] { offered_ = false; }) {}

    void tick() {
        constexpr auto instance_name = "mock_device_1";
        constexpr auto discovery_id = "mock-device-1";
        if (api_->getModuleInstance(instance_name)) {
            offered_ = false;
            return;
        }
        if (mute_handle_.refresh(*api_) || offered_) return;

        api_->requestInterfaceUi("dartwic.module-discovery", {
        {"presence", {{"available", true}}},
        {"discovery_id", discovery_id},
        {"device_type", "example_mock_device"},
        {"display_name", "Example Mock Device"},
        {"endpoint", {{"host", "simulated"}, {"port", 1}}},
        {"channels", nlohmann::json::array({
            {{"name", "mock_device_measurement"}, {"direction", "read"}, {"units", "test units"}},
            {{"name", "mock_device_command"}, {"direction", "write"}, {"units", "test units"}},
            {{"name", "mock_device_applied"}, {"direction", "read"}, {"units", "test units"}}
        })},
        {"provisioning", {
            {"module_type", "example_device"},
            {"suggested_instance_name", instance_name},
            {"tasks", nlohmann::json::array({
                {{"name_suffix", "_read"}, {"task_type", "mock_read"},
                 {"arguments", {
                    {"channel_prefix", "mock_device"},
                    {"measurement_channel", "mock_device_measurement"},
                    {"applied_channel", "mock_device_applied"},
                    {"connected_channel", "mock_device_connected"}
                 }}},
                {{"name_suffix", "_write"}, {"task_type", "mock_write"},
                 {"arguments", {
                    {"channel_prefix", "mock_device"},
                    {"command_channel", "mock_device_command"}
                 }}}
            })}
        }}
        }, {
            {"request_key", discovery_id},
            {"merge_key", "module-discovery"},
            {"silenceable", true},
            {"mute_scope", "engine"},
            {"notification_id", "device-discovery:mock-device-1"},
            {"reopen_completed", true}
        });
        offered_ = true;
    }

private:
    DARTWIC::API::SDK_API* api_;
    DARTWIC::API::NotificationMuteHandle mute_handle_;
    bool offered_ = false;
};
}
namespace Example {
    void ExampleDevicePlugin::onPluginLoaded() {
        registerRocketDriver(dartwic);
        dartwic->registerModuleType({
            .id = "example_device",
            .name = "Example Device"
        });

        auto mock_device_discovery = std::make_shared<MockDeviceDiscovery>(dartwic);
        dartwic->registerLoop("mock_device_discovery", "Example Mock Device Discovery", {
            .on_loop = [mock_device_discovery]() { mock_device_discovery->tick(); },
            .target_frequency_hz = 1.0
        });

        dartwic->registerPeer({
            .id = "example_flight_link",
            .name = "Example Flight Link",
            .default_config = {
                {"node_name", "external-program"},
                {"receive_endpoint", "tcp://127.0.0.1:17600"},
                {"send_endpoint", "tcp://127.0.0.1:17601"}
            },
            .protocol_id = "tempest.engine",
            .engine_protocol = true,
            .create = [](const nlohmann::json& config) {
                return std::make_shared<ExampleTransport>(config);
            },
            .configure = [](TEMPEST::Peer& peer) {
                peer.registerOperation({.name = "example/flight/ping", .display_name = "Flight link ping",
                    .description = "Echo a marker through the custom framed peer link.", .category = "Example peer",
                    .arguments = {{.name = "marker", .type = "string", .description = "Marker to echo."}}},
                    [](const TEMPEST::Value::Object& args) { return args; });
                peer.onTelemetry("example/flight/status", [](const TEMPEST::Value::Object&) {
                    // A real plugin can project these samples into channels or events here.
                });
            }
        });

        DARTWIC::API::TaskTypeDefinition task;
        task.metadata.structure = DARTWIC::API::TaskStructure::Periodic;
        task.metadata.default_arguments = {{"message", "Hello from the example plugin"}};
        task.on_task = [](const auto&, auto&, double) {};
        dartwic->registerTaskType("example_task", "Example Task", std::move(task));

        DARTWIC::API::TaskTypeDefinition fixed_demo;
        fixed_demo.metadata.structure = DARTWIC::API::TaskStructure::Periodic;
        fixed_demo.on_configure = [this](const auto&, DARTWIC::API::TaskRuntime& runtime) {
            using namespace DARTWIC::API;
            const std::string prefix = runtime.getTaskName();
            const std::string measurement = prefix + "_measurement";
            const std::string command = prefix + "_command";
            const std::string applied = prefix + "_applied";
            auto state = std::make_shared<FixedDemoState>();
            state->outputs[0] = dartwic->createFixedChannel(measurement, 0.0);
            state->command = dartwic->createFixedChannel(command, 0.0);
            state->outputs[1] = dartwic->createFixedChannel(applied, 0.0);
            for (const auto& channel : {measurement, command, applied}) {
                dartwic->upsertChannelField(channel, ChannelField::RECORD_MODE, RecordMode::Never, ChannelStorage::Fixed);
            }
            runtime.setFixedInputChannels({command});
            runtime.setTypedRuntimeContext("fixed-demo", state);
        };
        fixed_demo.on_start = [](const auto&, DARTWIC::API::TaskRuntime& runtime) {
            if (const auto state = runtime.getTypedRuntimeContext<FixedDemoState>("fixed-demo")) state->ticks = 0;
        };
        fixed_demo.on_task = [this](const auto&, DARTWIC::API::TaskRuntime& runtime, double) {
            const auto state = runtime.getTypedRuntimeContext<FixedDemoState>("fixed-demo");
            if (!state) return;
            state->commands[0] = dartwic->queryChannel(state->command);
            state->values[0] = ++state->ticks;
            state->values[1] = state->commands[0];
            for (size_t index = 0; index < state->outputs.size(); ++index)
                (void)dartwic->upsertChannel(state->outputs[index], state->values[index]);
        };
        dartwic->registerTaskType("fixed_driver_demo", "Fixed Driver Demo (simulated)", std::move(fixed_demo));

        DARTWIC::API::TaskTypeDefinition mock_read;
        mock_read.metadata.structure = DARTWIC::API::TaskStructure::Periodic;
        mock_read.metadata.default_arguments = {
            {"module_instance_name", ""}, {"channel_prefix", "mock_device"},
            {"measurement_channel", "mock_device_measurement"},
            {"applied_channel", "mock_device_applied"},
            {"connected_channel", "mock_device_connected"}
        };
        mock_read.on_configure = [this](const auto&, DARTWIC::API::TaskRuntime& runtime) {
            configureMockDriverTask(dartwic, runtime, false);
        };
        mock_read.on_start = [](const auto&, DARTWIC::API::TaskRuntime&) {};
        mock_read.on_task = [](const auto&, DARTWIC::API::TaskRuntime& runtime, double) {
            const auto state = runtime.getTypedRuntimeContext<MockDriverTaskState>("mock-driver-read");
            if (!state || !state->module || !state->module->ensureConnected()) return;
            const auto device = state->module->readDevice();
            state->values = {device.measurement, device.applied, 1.0};
            for (size_t index = 0; index < state->outputs.size(); ++index)
                (void)state->module->dartwic->upsertChannel(state->outputs[index], state->values[index]);
        };
        dartwic->registerTaskType("mock_read", "Mock Device Read", std::move(mock_read));

        DARTWIC::API::TaskTypeDefinition mock_write;
        mock_write.metadata.structure = DARTWIC::API::TaskStructure::Periodic;
        mock_write.metadata.default_arguments = {
            {"module_instance_name", ""}, {"channel_prefix", "mock_device"},
            {"command_channel", "mock_device_command"}
        };
        mock_write.on_configure = [this](const auto&, DARTWIC::API::TaskRuntime& runtime) {
            configureMockDriverTask(dartwic, runtime, true);
        };
        mock_write.on_start = [](const auto&, DARTWIC::API::TaskRuntime&) {};
        mock_write.on_task = [](const auto&, DARTWIC::API::TaskRuntime& runtime, double) {
            const auto state = runtime.getTypedRuntimeContext<MockDriverTaskState>("mock-driver-write");
            if (!state || !state->module || !state->module->ensureConnected()) return;
            state->command[0] = state->module->dartwic->queryChannel(state->command_handle);
            state->module->writeDevice(state->command[0]);
        };
        dartwic->registerTaskType("mock_write", "Mock Device Write", std::move(mock_write));

        dartwic->registerDCodeFunction(
            "test_value",
            "Test Value",
            [](const nlohmann::json& payload) {
                const double base = payload.is_object() ? payload.value("base", 40.0) : 40.0;
                const double offset = payload.is_object() ? payload.value("offset", 2.0) : 2.0;
                return nlohmann::json(base + offset);
            },
            "Return base + offset from the example plugin.",
            {
                {"base", "number", "Base value to add.", false},
                {"offset", "number", "Offset value to add.", false}
            },
            {
                {"value", "number", "The computed base + offset result.", true}
            }
        );

        dartwic->registerDCodeFunction(
            "test_outputs",
            "Test Outputs",
            [](const nlohmann::json& payload) {
                const double base = payload.is_object() ? payload.value("base", 40.0) : 40.0;
                const double offset = payload.is_object() ? payload.value("offset", 2.0) : 2.0;
                return nlohmann::json{
                    {"sum", base + offset},
                    {"difference", base - offset}
                };
            },
            "Return sum and difference fields from the example plugin.",
            {
                {"base", "number", "Base value used for both outputs.", false},
                {"offset", "number", "Offset value used for both outputs.", false}
            },
            {
                {"sum", "number", "The computed base + offset result.", true},
                {"difference", "number", "The computed base - offset result.", true}
            }
        );

        DARTWIC::API::DCodeFunctionArgument sample_element{
            "item", "number", "One numeric sample.", true};
        DARTWIC::API::DCodeFunctionArgument samples_argument{
            "samples", "list", "Three measurements to analyze.", true};
        samples_argument.list_length = 3;
        samples_argument.element = std::make_shared<DARTWIC::API::DCodeFunctionArgument>(sample_element);
        dartwic->registerDCodeFunction(
            "analyze_samples", "Analyze Samples",
            [](nlohmann::json& payload) {
                auto& samples = payload.at("samples");
                double sum = 0.0;
                double maximum = -std::numeric_limits<double>::infinity();
                for (const auto& sample : samples) {
                    const double value = sample.get<double>();
                    sum += value;
                    maximum = std::max(maximum, value);
                }
                samples[0] = samples.at(0).get<double>() + 10.0;
                return nlohmann::json{{"sum", sum}, {"maximum", maximum}};
            },
            "Return the sum and maximum of three samples.",
            {samples_argument},
            {{"sum", "number", "Sample sum.", true},
             {"maximum", "number", "Largest sample.", true}},
            [](DARTWIC::API::DCodePreparedValueView& input,
                DARTWIC::API::DCodePreparedMutableValue& result) {
                auto& samples = input.field("samples");
                double sum = 0.0;
                double maximum = -std::numeric_limits<double>::infinity();
                for (const auto& sample : samples.elements) {
                    const double value = sample.asNumber();
                    sum += value;
                    maximum = std::max(maximum, value);
                }
                result.field("sum").setNumber(sum);
                result.field("maximum").setNumber(maximum);
                samples.at(0).setNumber(samples.at(0).asNumber() + 10.0);
            }
        );

        dartwic->registerTelemetry(DARTWIC::API::TelemetryDefinition{
            .id = "echoed",
            .name = "Echoed payload",
            .description = "Arguments published by the example echo operation.",
            .delivery = "Published when the echo operation runs."
        });

        dartwic->registerOperation(DARTWIC::API::OperationDefinition{
            .id = "echo",
            .name = "Echo",
            .description = "Echo the arguments and publish them as telemetry.",
            .category = "Example peer",
            .handler = [this](const nlohmann::json& payload) {
                dartwic->publishTelemetry("echoed", payload);
                return nlohmann::json{{"echo", payload}};
            }
        });

        dartwic->registerLoop(
            "heartbeat",
            "Example Heartbeat",
            DARTWIC::API::PluginLoopDefinition{
                .on_loop = []() {},
                .target_frequency_hz = 10.0
            }
        );
    }

    DARTWIC::Modules::BaseModule* ExampleDevicePlugin::createModule(
        const std::string& module_type_id,
        nlohmann::json cfg,
        DARTWIC::API::SDK_API* api
    ) {
        if (module_type_id == "rocket_sim") return new RocketSimModule(std::move(cfg), api);
        if (module_type_id != "example_device") {
            return nullptr;
        }

        return new ExampleDeviceModule(cfg, api);
    }
}

DARTWIC_PLUGIN_EXPORT uint32_t dartwicEngineSdkAbiVersion() noexcept {
    return DARTWIC::Plugins::ENGINE_PLUGIN_SDK_ABI_VERSION;
}

DARTWIC_PLUGIN_EXPORT DARTWIC::Plugins::BasePlugin* createPlugin(
    nlohmann::json cfg,
    DARTWIC::API::SDK_API* api
) {
    return new Example::ExampleDevicePlugin(cfg, api);
}
