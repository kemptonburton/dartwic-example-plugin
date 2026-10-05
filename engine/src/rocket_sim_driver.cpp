#include "rocket_sim_module.h"
#include <set>
#include <stdexcept>

namespace Example {
void registerRocketDiscovery(DARTWIC::API::SDK_API* api);
namespace {
struct DriverState {
    std::shared_ptr<RocketSimModule> module;
    std::vector<std::string> inputs;
    std::vector<std::pair<size_t, std::string>> bindings;
    std::array<double, 5> faults{};
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
    for (const auto& row : args.at(argument)) {
        if (!row.is_object() || !row.value("device_field", nlohmann::json{}).is_string()
            || !row.value("channel", nlohmann::json{}).is_string())
            throw std::runtime_error(std::string(argument) + " needs device_field and channel strings");
        const auto field = row.at("device_field").get<std::string>();
        const auto channel = row.at("channel").get<std::string>();
        const auto found = std::find_if(fields.begin(), fields.end(), [&](const char* name) { return field == name; });
        if (found == fields.end() || channel.empty() || channel.find('|') != std::string::npos)
            throw std::runtime_error("Invalid rocket device binding: " + field);
        const auto index = static_cast<size_t>(found - fields.begin());
        if (!used.insert(index).second) throw std::runtime_error("Duplicate rocket device binding: " + field);
        result.emplace_back(index, channel);
    }
    return result;
}
std::shared_ptr<RocketSimModule> moduleFor(DARTWIC::API::SDK_API* api, DARTWIC::API::TaskRuntime& runtime) {
    const auto name = runtime.getArguments().value("module_instance_name", std::string{});
    auto module = std::dynamic_pointer_cast<RocketSimModule>(api->getModuleInstance(name));
    if (!module) throw std::runtime_error("Select a rocket_sim module for " + runtime.getTaskName());
    return module;
}
}
void registerRocketDriver(DARTWIC::API::SDK_API* api) {
    using namespace DARTWIC::API;
    api->registerModuleType({.id="rocket_sim", .name="Mock Rocket Device"});
    registerRocketDiscovery(api);
    for (bool writer : {false, true}) {
        TaskTypeDefinition task;
        task.metadata.structure = TaskStructure::Periodic;
        task.metadata.default_arguments = {{"module_instance_name", "rocket_device"}, {"channel_prefix", "rocket_sim"}};
        task.on_configure = [api, writer](const auto&, TaskRuntime& runtime) {
            moduleFor(api, runtime);
            const auto args = runtime.getArguments();
            const auto prefix = args.value("channel_prefix", std::string{"rocket_sim"});
            auto inputs = writer ? std::vector<std::string>{} : RocketSimModule::inputNames(prefix, false);
            const auto bindings = writer ? bindingsFor(args, "write_mappings", RocketSimModule::writeFields, prefix)
                : bindingsFor(args, "read_mappings", RocketSimModule::readFields, prefix);
            auto names = inputs;
            for (const auto& [index, channel] : bindings) names.push_back(channel);
            for (const auto& name : names) {
                api->upsertChannelField(name, ChannelField::VALUE,
                    api->queryChannelField(name, ChannelField::VALUE, 0.0), ChannelStorage::Fixed);
            }
            if (!writer) {
                const std::array<std::string, 24> units = {"mA", "K", "K", "K", "%", "%", "kg/s", "", "", "s"};
                for (const auto& [index, channel] : bindings) {
                    api->upsertChannelField(channel, ChannelField::UNITS, units[index]);
                    api->upsertChannelField(channel, ChannelField::CONTROL_OWNER, "task:" + runtime.getTaskName());
                    api->upsertChannelField(channel, ChannelField::ACTIVE_CONTROLLER, "task:" + runtime.getTaskName());
                    api->upsertChannelField(channel, ChannelField::CONTROL_POLICY, ControlPolicy::ObserveOnly);
                    api->upsertChannelField(channel, ChannelField::RECORD_MODE, RecordMode::OnValueChange);
                }
            }
            if (writer) for (const auto& [index, channel] : bindings) inputs.push_back(channel);
            runtime.setFixedInputChannels(inputs);
        };
        task.on_start = [api, writer](const auto&, TaskRuntime& runtime) {
            auto state = std::make_shared<DriverState>();
            state->module = moduleFor(api, runtime);
            const auto args = runtime.getArguments();
            const auto prefix = args.value("channel_prefix", std::string{"rocket_sim"});
            if (!writer) state->inputs = RocketSimModule::inputNames(prefix, false);
            state->bindings = writer ? bindingsFor(args, "write_mappings", RocketSimModule::writeFields, prefix)
                : bindingsFor(args, "read_mappings", RocketSimModule::readFields, prefix);
            runtime.setTypedRuntimeContext("rocket-driver", state);
            api->writeLog("Rocket device", writer ? "Write task started" : "Read task started");
        };
        task.on_task = [api, writer](const auto&, TaskRuntime& runtime, double) {
            const auto state = runtime.getTypedRuntimeContext<DriverState>("rocket-driver");
            if (!state) throw std::runtime_error("Rocket driver has no initialized task context");
            if (writer) {
                state->commands.fill(0);
                for (const auto& [index, channel] : state->bindings)
                    state->commands[index] = api->queryChannelField(channel, ChannelField::VALUE, 0.0);
                state->module->write(state->commands);
            } else {
                for (size_t index = 0; index < state->inputs.size(); ++index)
                    state->faults[index] = api->queryChannelField(state->inputs[index], ChannelField::VALUE, 0.0);
                const auto values = state->module->read(state->faults);
                for (const auto& [index, channel] : state->bindings)
                    api->upsertChannelField(channel, ChannelField::VALUE, values[index]);
            }
        };
        task.on_end = [writer](const auto&, TaskRuntime& runtime) {
            if (writer) if (const auto state = runtime.getTypedRuntimeContext<DriverState>("rocket-driver")) state->module->stopOutputs();
        };
        api->registerTaskType(writer ? "rocket_write" : "rocket_read", writer ? "Mock Rocket Write" : "Mock Rocket Read", std::move(task));
    }
}
}
