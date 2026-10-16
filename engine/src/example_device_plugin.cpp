#include "example_device_plugin.h"
#include "example_transport.h"
#include "example_device_module.h"
#include "stress_device_module.h"
#include <dartwic/EngineProtocol.h>
#include <tempest/Peer.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
namespace Example {
    void ExampleDevicePlugin::onPluginLoaded() {
        registerExampleDeviceDriver(dartwic);
        registerStressDevice(dartwic);

        dartwic->registerPeer({
            .id = "example_flight_link",
            .name = "Example Flight Link",
            .default_config = {
                {"node_name", "external-program"},
                {"receive_endpoint", "tcp://127.0.0.1:17600"},
                {"send_endpoint", "tcp://127.0.0.1:17601"}
            },
            .protocol_id = "tempest.engine",
            .protocol_version = DARTWIC::EngineContract::version,
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

        DARTWIC::API::DCodeFunctionArgument samples_argument{
            "samples", "list", "Three numeric samples to analyze and mutate.", true};
        samples_argument.list_length = 3;
        samples_argument.element = std::make_shared<DARTWIC::API::DCodeFunctionArgument>(
            DARTWIC::API::DCodeFunctionArgument{"item", "number", "One sample.", true});
        dartwic->registerDCodeFunction(
            "analyze_samples",
            "Analyze Samples",
            [](nlohmann::json& payload) {
                auto& samples = payload.at("samples");
                const double first = samples.at(0).get<double>();
                const double second = samples.at(1).get<double>();
                const double third = samples.at(2).get<double>();
                samples[0] = first + 10.0;
                return nlohmann::json{{"sum", first + second + third},
                    {"maximum", std::max({first, second, third})}};
            },
            "Return the sum and maximum of three samples, then add ten to the first sample.",
            {samples_argument},
            {{"sum", "number", "Sum before mutation.", true},
             {"maximum", "number", "Maximum before mutation.", true}},
            [](DARTWIC::API::DCodePreparedValueView& input,
                DARTWIC::API::DCodePreparedMutableValue& result) {
                auto samples = input.field("samples").elements;
                const double first = samples[0].asNumber();
                const double second = samples[1].asNumber();
                const double third = samples[2].asNumber();
                result.field("sum").setNumber(first + second + third);
                result.field("maximum").setNumber(std::max({first, second, third}));
                samples[0].setNumber(first + 10.0);
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
        if (module_type_id == "stress_device") return new StressDeviceModule(cfg, api);
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
