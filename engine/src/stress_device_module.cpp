#include "stress_device_module.h"
#include <cmath>
#include <chrono>
#include <limits>
#include <stdexcept>

namespace Example {
double StressDeviceModule::read() {
    std::scoped_lock lock(mutex_);
    const int maximum = maximum_;
    value_ += direction_;
    if (value_ >= maximum) { value_ = maximum; direction_ = -1; latched_ = 1; }
    if (value_ <= 1) { value_ = 1; direction_ = 1; latched_ = 0; }
    ++reads_;
    return value_;
}
void StressDeviceModule::write(const std::array<double, 5>& values) {
    std::scoped_lock lock(mutex_);
    registers_ = values;
    ++writes_;
    if (!reads_) return;
    const double first = value_ == 1 ? 2.0 : value_ > threshold_ ? 1.0 : latched_;
    for (size_t i = 0; i < values.size(); ++i) {
        const double expected = i == 0 ? first : latched_;
        if (values[i] == expected) continue;
        ++mismatches_;
        if (examples_.size() < 16) examples_.push_back({{"read_cycle", reads_}, {"write_cycle", writes_},
            {"input", value_}, {"output", i + 1}, {"expected", expected}, {"actual", values[i]}});
    }
}
nlohmann::json StressDeviceModule::status() {
    std::scoped_lock lock(mutex_);
    return {{"reads", reads_}, {"writes", writes_}, {"mismatches", mismatches_},
        {"input", value_}, {"registers", registers_}, {"examples", examples_},
        {"steady_ns", std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()}};
}
namespace {
struct State {
    std::shared_ptr<StressDeviceModule> module;
    DARTWIC::API::SDK_API* api = nullptr;
    std::array<DARTWIC::API::FixedChannelHandle, 5> channels;
};
}
void registerStressDevice(DARTWIC::API::SDK_API* api) {
    using namespace DARTWIC::API;
    api->registerModuleType({.id="stress_device", .name="Stress Device"});
    for (const bool writer : {false, true}) {
        TaskTypeDefinition task;
        task.metadata.structure = TaskStructure::Periodic;
        task.metadata.default_arguments = {{"module_instance_name", "stress_0000"}, {"channel_prefix", "stress_0000"}};
        task.on_configure = [api, writer](const auto&, TaskRuntime& runtime) {
            auto state = std::make_shared<State>();
            const auto args = runtime.getArguments();
            state->module = std::dynamic_pointer_cast<StressDeviceModule>(api->getModuleInstance(args.at("module_instance_name").template get<std::string>()));
            if (!state->module) throw std::runtime_error("Select a stress_device module.");
            state->api = state->module->dartwic;
            const auto record_mode = args.value("record_mode", std::string{"on_value_change"});
            if (record_mode != "on_value_change" && record_mode != "every_value") throw std::runtime_error("Invalid stress record mode.");
            const auto key = writer ? "write_mappings" : "read_mappings";
            if (!args.contains(key) || !args.at(key).is_array() || args.at(key).size() != 5)
                throw std::runtime_error("Map all five stress device registers.");
            std::array<bool, 5> used{};
            std::array<std::string, 5> names;
            for (const auto& row : args.at(key)) {
                const int index = row.at("register").template get<int>();
                const auto name = row.at("channel").template get<std::string>();
                if (index < 0 || index >= 5 || used[index] || name.empty() || name.find('|') != std::string::npos)
                    throw std::runtime_error("Invalid or duplicate stress register mapping.");
                for (const auto& previous : names) if (previous == name) throw std::runtime_error("Duplicate stress channel binding.");
                used[index] = true;
                names[index] = name;
            }
            std::vector<std::string> inputs;
            for (size_t i = 0; i < 5; ++i) {
                const auto& name = names[i];
                state->channels[i] = state->api->createFixedChannel(name);
                state->api->upsertChannelField(name, ChannelField::RECORD_MODE, record_mode == "every_value" ? RecordMode::EveryValue : RecordMode::OnValueChange);
                if (writer) inputs.push_back(name);
            }
            runtime.setFixedInputChannels(inputs);
            runtime.setTypedRuntimeContext("stress", state);
        };
        task.on_task = [writer](const auto&, TaskRuntime& runtime, double) {
            const auto state = runtime.getTypedRuntimeContext<State>("stress");
            if (writer) {
                std::array<double, 5> values;
                for (size_t i = 0; i < 5; ++i) values[i] = state->api->queryChannel(state->channels[i], std::numeric_limits<double>::quiet_NaN());
                state->module->write(values);
            } else {
                const auto value = state->module->read();
                for (const auto& channel : state->channels)
                    if (!state->api->upsertChannel(channel, value)) throw std::runtime_error("Stress input write rejected.");
            }
        };
        api->registerTaskType(writer ? "stress_write" : "stress_read", writer ? "Stress Device Write" : "Stress Device Read", std::move(task));
    }
    api->registerOperation("stress_status", "Stress device verification", [api](const nlohmann::json& payload) {
        nlohmann::json result = nlohmann::json::object();
        for (const auto& name : payload.at("modules")) {
            const auto key = name.get<std::string>();
            auto module = std::dynamic_pointer_cast<StressDeviceModule>(api->getModuleInstance(key));
            if (!module) throw std::runtime_error("Unknown stress module: " + key);
            result[key] = module->status();
        }
        return result;
    });
}
}
