#include "example_device_module.h"
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace Example {
void registerExampleDeviceDiscovery(DARTWIC::API::SDK_API* api);
namespace {
struct DriverState {
    std::shared_ptr<ExampleDeviceModule> module;
    std::vector< DARTWIC::API::FixedChannelHandle> inputs;
    std::vector<std::pair<size_t, DARTWIC::API::FixedChannelHandle>> bindings;
    std::array<double, 5> simulationInputs{};
    std::array<double, 9> commands{};
};
template<size_t N>
std::vector<std::pair<size_t, std::string>> bindingsFor(const nlohmann::json& args,
    const char* argument, const std::array<const char*, N>& fields, const std::string& prefix) {
    std::vector<std::pair<size_t, std::string>> result;
    if (!args.contains(argument)) {
        for (size_t index = 0; index < fields.size(); ++index)
            result.emplace_back(index, prefix + "_" + fields[index]);
        return result;
    }
    if (!args.at(argument).is_array()) throw std::runtime_error(std::string(argument) + " must be a binding array");
    std::set<size_t> used;
    std::set<std::string> channels;
    for (const auto& row : args.at(argument)) {
        if (!row.is_object() || !row.value("device_field", nlohmann::json{}).is_string()
            || !row.value("channel", nlohmann::json{}).is_string())
            throw std::runtime_error(std::string(argument) + " needs device_field and channel strings");
        const auto field = row.at("device_field").get<std::string>();
        const auto channel = row.at("channel").get<std::string>();
        const auto found = std::find_if(fields.begin(), fields.end(), [&](const char* name) { return field == name; });
        if (found == fields.end() || channel.empty() || channel.find('|') != std::string::npos)
            throw std::runtime_error("Invalid example device binding: " + field);
        const auto index = static_cast<size_t>(found - fields.begin());
        if (!used.insert(index).second) throw std::runtime_error("Duplicate example device binding: " + field);
        if (!channels.insert(channel).second) throw std::runtime_error("Duplicate example device channel: " + channel);
        result.emplace_back(index, channel);
    }
    return result;
}
std::shared_ptr<ExampleDeviceModule> moduleFor(DARTWIC::API::SDK_API* api, DARTWIC::API::TaskRuntime& runtime) {
    const auto name = runtime.getArguments().value("module_instance_name", std::string{});
    auto module = std::dynamic_pointer_cast<ExampleDeviceModule>(api->getModuleInstance(name));
    if (!module) throw std::runtime_error("Select an example_device module for " + runtime.getTaskName());
    return module;
}
}
void registerExampleDeviceDriver(DARTWIC::API::SDK_API* api) {
    using namespace DARTWIC::API;
    api->registerModuleType({.id="example_device", .name="Example Device"});
    registerExampleDeviceDiscovery(api);
    for (bool writer : {false, true}) {
        TaskTypeDefinition task;
        task.metadata.structure = TaskStructure::Periodic;
        task.metadata.default_arguments = {{"module_instance_name", "example_device_1"}, {"channel_prefix", "example_device"}};
        task.on_configure = [api, writer](const auto&, TaskRuntime& runtime) {
            auto state = std::make_shared<DriverState>();
            state->module = moduleFor(api, runtime);
            const auto args = runtime.getArguments();
            const auto prefix = args.value("channel_prefix", std::string{"example_device"});
            auto inputs = writer ? std::vector<std::string>{} : ExampleDeviceModule::inputNames(prefix, false);
            const auto bindings = writer ? bindingsFor(args, "write_mappings", ExampleDeviceModule::writeFields, prefix)
                : bindingsFor(args, "read_mappings", ExampleDeviceModule::readFields, prefix);
            for (const auto& name : inputs) state->inputs.push_back(api->createFixedChannel(name));
            if (!writer) api->upsertChannelField(prefix + "_run_fill", ChannelField::STALE_TIMEOUT, 1.0);
            for (const auto& [index, channel] : bindings) {
                const auto handle = api->createFixedChannel(channel);
                state->bindings.emplace_back(index, handle);
                if (writer) continue;
                const std::array<std::string, 22> units = {"mA", "K", "K", "K", "kg/s", "", "", "s"};
                api->upsertChannelField(channel, ChannelField::UNITS, units[index]);
                api->upsertChannelField(channel, ChannelField::STALE_TIMEOUT, 1.0);
                api->setChannel(channel);
                api->upsertChannelField(channel, ChannelField::RECORD_MODE, RecordMode::OnValueChange);
            }
            if (writer) for (const auto& [index, channel] : bindings) inputs.push_back(channel);
            runtime.setFixedInputChannels(inputs);
            runtime.setTypedRuntimeContext("dev", state);
        };
        task.on_start = [api, writer](const auto&, TaskRuntime&) {
            api->writeLog("Example device", writer ? "Write task started" : "Read task started");
        };
        task.on_task = [api, writer](const auto&, TaskRuntime& runtime, double) {
            const auto state = runtime.getTypedRuntimeContext<DriverState>("dev");
            if (!state) throw std::runtime_error("Example device driver has no initialized task context");
            if (writer) {
                state->commands.fill(0);
                for (const auto& [index, handle] : state->bindings) {
                    const double command = api->queryChannel(handle, std::numeric_limits<double>::quiet_NaN());
                    if (std::isnan(command)) throw std::runtime_error("Example device command binding is unavailable.");
                    state->commands[index] = command;
                }
                state->module->write(state->commands);
            } else {
                for (size_t index = 0; index < state->inputs.size(); ++index) {
                    const double input = api->queryChannel(state->inputs[index], std::numeric_limits<double>::quiet_NaN());
                    if (std::isnan(input)) throw std::runtime_error("Example device simulation input is unavailable.");
                    state->simulationInputs[index] = input;
                }
                const auto values = state->module->read(state->simulationInputs);
                for (const auto& [index, handle] : state->bindings)
                    if (!api->upsertChannel(handle, values[index]))
                        throw std::runtime_error("Example device driver fixed-channel write was denied or its handle became stale.");
            }
        };
        task.on_end = [writer](const auto&, TaskRuntime& runtime) {
            if (writer) if (const auto state = runtime.getTypedRuntimeContext<DriverState>("dev")) state->module->stopOutputs();
        };
        api->registerTaskType(writer ? "example_write" : "example_read", writer ? "Example Device Write" : "Example Device Read", std::move(task));
    }
}
}
