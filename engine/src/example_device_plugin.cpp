#include "example_device_plugin.h"
#include "example_share_transport.h"
#include <array>

namespace {
// A simulated acquisition/write pair for the fixed-channel driver guide.
// No device or networking guarantee is implied by the applied-value echo.
struct FixedDemoState {
    DARTWIC::API::FixedChannelBatch inputs;
    DARTWIC::API::FixedChannelBatch outputs;
    std::array<double, 1> commands{};
    std::array<double, 2> values{};
    double ticks = 0;
};
}
namespace Example {
    void ExampleDevicePlugin::onPluginLoaded() {
        dartwic->registerModuleType({
            .id = "example_device",
            .name = "Example Device"
        });

        dartwic->registerShareTransport({
            .id = "example_flight_link",
            .name = "Example Flight Link",
            .default_config = {
                {"node_name", "external-program"},
                {"receive_endpoint", "tcp://127.0.0.1:17600"},
                {"send_endpoint", "tcp://127.0.0.1:17601"}
            },
            .create = [](const nlohmann::json& config) {
                return std::make_shared<ExampleShareTransport>(config);
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
            for (const auto& channel : {measurement, command, applied}) {
                dartwic->insertChannelField(channel, ChannelField::VALUE, 0.0, ChannelStorage::Fixed);
                dartwic->upsertChannelField(channel, ChannelField::RECORD_MODE, RecordMode::Never, ChannelStorage::Fixed);
            }
            runtime.setFixedInputChannels({command});
            auto state = std::make_shared<FixedDemoState>();
            state->inputs = dartwic->resolveFixedChannels({command});
            state->outputs = dartwic->resolveFixedChannels({measurement, applied});
            runtime.setTypedRuntimeContext("fixed-demo", state);
        };
        fixed_demo.on_start = [](const auto&, DARTWIC::API::TaskRuntime& runtime) {
            if (const auto state = runtime.getTypedRuntimeContext<FixedDemoState>("fixed-demo")) state->ticks = 0;
        };
        fixed_demo.on_task = [this](const auto&, DARTWIC::API::TaskRuntime& runtime, double) {
            const auto state = runtime.getTypedRuntimeContext<FixedDemoState>("fixed-demo");
            if (!state) return;
            dartwic->queryFixedChannelValues(state->inputs, state->commands);
            state->values[0] = ++state->ticks;
            state->values[1] = state->commands[0];
            dartwic->upsertFixedChannelValues(state->outputs, state->values);
        };
        dartwic->registerTaskType("fixed_driver_demo", "Fixed Driver Demo (simulated)", std::move(fixed_demo));

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

        dartwic->registerOperation(
            "echo",
            "Echo",
            [](const nlohmann::json& payload) {
                return nlohmann::json{{"echo", payload}};
            }
        );

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
        if (module_type_id != "example_device") {
            return nullptr;
        }

        return new ExampleDeviceModule(cfg, api);
    }
}

DARTWIC_PLUGIN_EXPORT DARTWIC::Plugins::BasePlugin* createPlugin(
    nlohmann::json cfg,
    DARTWIC::API::SDK_API* api
) {
    return new Example::ExampleDevicePlugin(cfg, api);
}
