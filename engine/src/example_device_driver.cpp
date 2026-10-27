#include "example_device_module.h"
#include "stress_device_module.h"
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

namespace Example {
void registerExampleDeviceDiscovery(DARTWIC::API::SDK_API* api);
namespace {
struct DriverState {
    std::shared_ptr<ExampleDeviceModule> module;
    std::vector< DARTWIC::API::FixedChannelHandle> inputs;
    std::vector<std::pair<size_t, DARTWIC::API::FixedChannelHandle>> readBindings;
    struct WriteBinding {
        size_t index;
        DARTWIC::API::FixedChannelHandle command;
        DARTWIC::API::FixedChannelHandle readback;
    };
    std::vector<WriteBinding> writeBindings;
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
        // Older task files may still contain this module-owned status row.
        // Keep them loadable, but never let a read task claim or publish it.
        if (std::string(argument) == "read_mappings" && field == "connected") continue;
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
std::vector<std::tuple<size_t, std::string, std::string>> writeBindingsFor(
    const nlohmann::json& args, const std::string& prefix) {
    const auto commands = bindingsFor(args, "write_mappings", ExampleDeviceModule::writeFields, prefix);
    std::vector<std::tuple<size_t, std::string, std::string>> result;
    std::set<std::string> channels;
    for (const auto& [index, command] : commands) channels.insert(command);
    for (const auto& [index, command] : commands) {
        std::string readback = command + "_state";
        if (args.contains("write_mappings")) {
            for (const auto& row : args.at("write_mappings")) {
                if (row.at("device_field") != ExampleDeviceModule::writeFields[index]) continue;
                if (row.contains("readback_channel")) {
                    if (!row.at("readback_channel").is_string())
                        throw std::runtime_error("Example device readback_channel must be a string");
                    readback = row.at("readback_channel").get<std::string>();
                }
                break;
            }
        }
        if (readback.empty() || readback.find('|') != std::string::npos || !channels.insert(readback).second)
            throw std::runtime_error("Invalid or duplicate example device readback channel: " + readback);
        result.emplace_back(index, command, readback);
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
    struct ConnectionPublisher {
        std::weak_ptr<DARTWIC::Modules::BaseModule> module;
        FixedChannelHandle channel;
    };
    auto publishers = std::make_shared<std::unordered_map<std::string, ConnectionPublisher>>();
    api->registerLoop("connection_monitor", "Example Device Connections", {
        .on_start = [publishers] { publishers->clear(); },
        .on_loop = [api, publishers] {
            // This background monitor is independent of device read/write tasks.
            // Prune deleted/replaced modules before adopting their replacements.
            for (auto it = publishers->begin(); it != publishers->end();) {
                if (it->second.module.expired() ||
                    api->getModuleInstance(it->first) != it->second.module.lock()) {
                    api->freeChannel(it->second.channel);
                    it = publishers->erase(it);
                } else ++it;
            }
            for (const auto& summary : api->getModuleInstances("example_device_plugin")) {
                const auto module = api->getModuleInstance(summary.name);
                const auto device = std::dynamic_pointer_cast<ExampleDeviceModule>(module);
                const auto stress = std::dynamic_pointer_cast<StressDeviceModule>(module);
                if (!device && !stress) continue;
                auto [it, inserted] = publishers->try_emplace(summary.name,
                    ConnectionPublisher{module, device ? device->connectionChannel() : stress->connectionChannel()});
                const double connected = device ? device->monitorConnection() : 1.0;
                api->setChannel(it->second.channel, connected);
            }
        },
        .target_frequency_hz = 1.0,
    });
    for (bool writer : {false, true}) {
        TaskTypeDefinition task;
        task.metadata.structure = TaskStructure::Periodic;
        task.metadata.default_arguments = {{"module_instance_name", "example_device_1"}, {"channel_prefix", "example_device"}};
        task.on_configure = [api, writer](const auto&, TaskRuntime& runtime) {
            auto state = std::make_shared<DriverState>();
            state->module = moduleFor(api, runtime);
            const auto args = runtime.getArguments();
            const auto prefix = args.value("channel_prefix", std::string{"example_device"});
            std::vector<std::string> inputs;
            if (writer) {
                for (const auto& [index, command, readback] : writeBindingsFor(args, prefix)) {
                    state->writeBindings.push_back({index, api->createFixedChannel(command), api->createFixedChannel(readback)});
                    inputs.push_back(command);
                    api->upsertChannelField(readback, ChannelField::STALE_TIMEOUT, 1.0);
                    api->upsertChannelField(readback, ChannelField::RECORD_MODE, RecordMode::OnValueChange);
                }
            } else {
                inputs = ExampleDeviceModule::simulationInputNames(
                    state->module->getParameter<std::string>("channel_prefix", "example_device"));
                for (const auto& name : inputs) state->inputs.push_back(state->module->dartwic->createFixedChannel(name));
                const auto bindings = bindingsFor(args, "read_mappings", ExampleDeviceModule::readFields, prefix);
                for (const auto& [index, channel] : bindings) {
                    state->readBindings.emplace_back(index, api->createFixedChannel(channel));
                    const std::array<std::string, 8> units = {"mA", "K", "K", "K", "kg/s", "", "s", ""};
                    api->upsertChannelField(channel, ChannelField::UNITS, units[index]);
                    api->upsertChannelField(channel, ChannelField::STALE_TIMEOUT, 1.0);
                    api->upsertChannelField(channel, ChannelField::RECORD_MODE, RecordMode::OnValueChange);
                }
            }
            runtime.setFixedInputChannels(inputs);
            runtime.setTypedRuntimeContext("dev", state);
        };
        task.on_start = [api, writer](const auto&, TaskRuntime& runtime) {
            const auto state = runtime.getTypedRuntimeContext<DriverState>("dev");
            if (!state) throw std::runtime_error("Example device task is not configured.");
            if (writer) for (const auto& binding : state->writeBindings) api->setChannel(binding.readback);
            else for (const auto& [index, channel] : state->readBindings) api->setChannel(channel);
            api->writeLog("Example device", writer ? "Write task started" : "Read task started");
        };
        task.on_task = [api, writer](const auto&, TaskRuntime& runtime, double) {
            const auto state = runtime.getTypedRuntimeContext<DriverState>("dev");
            if (!state) throw std::runtime_error("Example device driver has no initialized task context");
            if (writer) {
                state->commands.fill(0);
                for (const auto& binding : state->writeBindings) {
                    const double command = api->queryChannel(binding.command, std::numeric_limits<double>::quiet_NaN());
                    if (std::isnan(command)) throw std::runtime_error("Example device command binding is unavailable.");
                    state->commands[binding.index] = command;
                }
                const auto states = state->module->write(state->commands);
                for (const auto& binding : state->writeBindings)
                    if (!api->upsertChannel(binding.readback, states[binding.index]))
                        throw std::runtime_error("Example device command-state write was denied or its handle became stale.");
            } else {
                for (size_t index = 0; index < state->inputs.size(); ++index) {
                    const double input = api->queryChannel(state->inputs[index], std::numeric_limits<double>::quiet_NaN());
                    if (std::isnan(input)) throw std::runtime_error("Example device simulation input is unavailable.");
                    state->simulationInputs[index] = input;
                }
                const auto values = state->module->read(state->simulationInputs);
                for (const auto& [index, handle] : state->readBindings)
                    if (!api->upsertChannel(handle, values[index]))
                        throw std::runtime_error("Example device driver fixed-channel write was denied or its handle became stale.");
            }
        };
        task.on_end = [api, writer](const auto&, TaskRuntime& runtime) {
            if (writer) if (const auto state = runtime.getTypedRuntimeContext<DriverState>("dev")) {
                state->module->stopOutputs();
                for (const auto& binding : state->writeBindings) api->upsertChannel(binding.readback, 0.0);
            }
        };
        api->registerTaskType(writer ? "example_write" : "example_read", writer ? "Example Device Write" : "Example Device Read", std::move(task));
    }
}
}
