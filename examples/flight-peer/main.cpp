#include <tempest/ZmqPeer.h>
#include <tempest/JsonValue.h>
#include <dartwic/EngineProtocol.h>
#include "example_transport.h"
#include <atomic>
#include <cmath>
#include <csignal>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;
namespace {
std::atomic<bool> running{true};
void stop(int) { running = false; }
uint64_t timestamp() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
double number(const TEMPEST::Value& value) {
    const auto json = TEMPEST::Json::fromValue(value);
    if (!json.is_number()) throw TEMPEST::RemoteError("A numeric value is required", "invalid_argument");
    const auto n = json.get<double>();
    if (!std::isfinite(n)) throw TEMPEST::RemoteError("The value must be finite", "invalid_argument");
    return n;
}
}
int main(int argc, char** argv) {
    try {
        std::string transport = "custom", host = "127.0.0.1", password;
        std::string receive_endpoint = "tcp://127.0.0.1:17601", send_endpoint = "tcp://127.0.0.1:17600";
        int port = 7400, seconds = 0;
        for (int i=1; i<argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--help") {
                std::cout << "rocket-flight-peer --transport custom|native [--host 127.0.0.1] [--port 7400] [--seconds N]\n"
                          << "Native mode reads DARTWIC_PASSWORD from the environment.\n"
                          << "Custom: --receive-endpoint tcp://127.0.0.1:17601 --send-endpoint tcp://127.0.0.1:17600\n";
                return 0;
            }
            if (i+1 >= argc) throw std::invalid_argument("Missing value for " + argument);
            const std::string value = argv[++i];
            if (argument == "--transport") transport = value;
            else if (argument == "--host") host = value;
            else if (argument == "--port") port = std::stoi(value);
            else if (argument == "--seconds") seconds = std::stoi(value);
            else if (argument == "--receive-endpoint") receive_endpoint = value;
            else if (argument == "--send-endpoint") send_endpoint = value;
            else throw std::invalid_argument("Unknown argument " + argument);
        }
        if (const auto secret = std::getenv("DARTWIC_PASSWORD")) password = secret;
        if (transport != "custom" && transport != "native") throw std::invalid_argument("Choose custom or native transport");
        if (transport == "native" && password.empty()) throw std::invalid_argument("Set DARTWIC_PASSWORD for the native peer");
        TEMPEST::TransportPtr link;
        if (transport == "native") link = TEMPEST::makeZmqTransport({.host=host, .port=port, .password=password});
        else link = std::make_shared<Example::ExampleTransport>(nlohmann::json{
            {"receive_endpoint", receive_endpoint}, {"send_endpoint", send_endpoint}});
        TEMPEST::Peer peer({.node_name="FLIGHT_COMPUTER",
            .peer_id=transport == "native" ? "tempest.engine" : "example_device_plugin.example_flight_link",
            .protocol_id="tempest.engine"}, link);
        DARTWIC::EngineProtocol protocol(peer);
        std::mutex mutex;
        std::atomic<double> rate{5}, ambient{295};
        std::atomic<uint64_t> counter{0}, ground_snapshots{0};
        std::string mode = "standby";
        std::deque<DARTWIC::LogEntry> history;
        uint64_t sequence = 0;
        const auto log = [&](const std::string& text) {
            DARTWIC::LogEntry entry{.owner_node="FLIGHT_COMPUTER", .session=peer.sessionId(),
                .stream="Flight computer", .text=text, .timestamp_ns=timestamp()};
            { std::lock_guard lock(mutex); entry.sequence=++sequence; history.push_back(entry); if(history.size()>250) history.pop_front(); }
            std::cout << text << std::endl;
            protocol.logs().publish(std::move(entry));
        };
        const auto snapshots = [&] {
            std::vector<DARTWIC::ChannelSnapshot> result;
            const auto add = [&](std::string key, TEMPEST::Value value, std::string units, bool writable=false) {
                result.push_back({"FLIGHT_COMPUTER", key,
                    {{"value", value}, {"units", units}, {"timestamp", timestamp()},
                     {"control_policy", writable ? "free" : "observe_only"}, {"control_owner", writable ? "" : "peer:flight"},
                     {"record_mode", "on_value_change"}, {"stale_timeout", 2.0}, {"storage", "dynamic"}},
                    counter.load(), peer.sessionId()});
            };
            add("battery_voltage", 24.0 + std::sin(counter.load()*0.01)*0.2, "V");
            add("sample_counter", counter.load(), "samples");
            add("telemetry_rate_hz", rate.load(), "Hz", true);
            add("ground_ambient", ambient.load(), "K");
            add("ground_snapshot_counter", ground_snapshots.load(), "snapshots");
            return result;
        };
        DARTWIC::ChannelHandlers channels;
        channels.query = [&](const auto& request) {
            auto records = snapshots();
            if (!request.channels.empty()) std::erase_if(records, [&](const auto& record) {
                return std::find(request.channels.begin(), request.channels.end(), record.channel) == request.channels.end();
            });
            return DARTWIC::ChannelQueryResult{std::move(records)};
        };
        channels.upsert = [&](const DARTWIC::ChannelUpsert& request) {
            if (request.owner_node != "FLIGHT_COMPUTER" || request.channel != "telemetry_rate_hz" || request.field != "value")
                throw TEMPEST::RemoteError("Only telemetry_rate_hz is writable", "permission_denied");
            const auto value = number(request.value);
            if (value < 1 || value > 20) throw TEMPEST::RemoteError("Telemetry rate must be 1 through 20 Hz", "invalid_argument");
            rate = value;
            log("Ground commanded telemetry rate: " + std::to_string(value) + " Hz");
        };
        channels.snapshot = [&](const DARTWIC::ChannelSnapshotTelemetry& snapshot) {
            if (++ground_snapshots == 1) log("Received first ground channel snapshot");
            if (const auto entry = snapshot.channels.find("rocket_sim_ground_ambient"); entry != snapshot.channels.end()) {
                const auto data = entry->second.object();
                if (const auto v = data.find("value"); v != data.end()) ambient = number(v->second);
            }
        };
        channels.telemetry = [&](const DARTWIC::ChannelTelemetry& update) {
            if (update.kind == DARTWIC::ChannelTelemetry::Kind::Upsert && update.upsert.channel == "rocket_sim_ground_ambient") ambient = number(update.upsert.value);
        };
        protocol.channels().setHandlers(std::move(channels));
        protocol.logs().setHandlers({
            .query=[&](const DARTWIC::LogQuery& query) {
                std::vector<DARTWIC::LogEntry> result;
                std::lock_guard lock(mutex);
                for (const auto& entry:history) if ((query.stream.empty() || query.stream==entry.stream) &&
                    (query.owner_node.empty() || query.owner_node==entry.owner_node) &&
                    (query.session.empty() || query.session==entry.session) &&
                    (!query.after || entry.sequence>query.after) && (!query.before || entry.sequence<query.before) &&
                    (query.text.empty() || entry.text.find(query.text)!=std::string::npos)) result.push_back(entry);
                if (result.size()>query.limit) result.erase(result.begin(), result.end()-query.limit);
                return result;
            },
            .list=[&] { return std::vector<DARTWIC::LogStream>{{"FLIGHT_COMPUTER",peer.sessionId(),"Flight computer"}}; }
        });
        DARTWIC::ArgusEvent event{.event_id="flight-link-ready", .title="Mock flight computer ready",
            .description="The standalone flight peer completed registration. Its channels, operations and log stream are available.",
            .owner_node="FLIGHT_COMPUTER", .timestamp=timestamp(), .channels={"FLIGHT_COMPUTER:battery_voltage"}};
        protocol.argus().setHandlers({
            .query=[&](const auto&) { std::lock_guard lock(mutex); return DARTWIC::ArgusQueryResult{{event},1,1}; },
            .action=[&](const DARTWIC::ArgusAction& action) {
                if (action.event_id!=event.event_id) throw TEMPEST::RemoteError("Unknown event", "not_found");
                std::lock_guard lock(mutex);
                if (action.kind==DARTWIC::ArgusAction::Kind::UpdateStatus) event.status=action.payload.at("status").string();
                else if (action.kind==DARTWIC::ArgusAction::Kind::Delete) event.status="deleted";
                else throw TEMPEST::RemoteError("Unsupported event action", "unsupported");
                return DARTWIC::ArgusActionResult{event.event_id,{{"status",event.status}}};
            }
        });
        peer.registerOperation({.name="flight/set-mode", .display_name="Set mock flight mode",
            .description="Apply a mode to the simulated flight computer and log it.", .category="Mock flight computer",
            .arguments={{.name="mode",.type="string",.description="standby or test",.required=true}}},
            [&](const TEMPEST::Value::Object& arguments) {
                const auto value=arguments.at("mode").string();
                if (value!="standby" && value!="test") throw TEMPEST::RemoteError("Choose standby or test", "invalid_argument");
                { std::lock_guard lock(mutex); mode=value; }
                log("Mode applied: " + value);
                return TEMPEST::Value::Object{{"applied_mode",value}};
            });
        std::signal(SIGINT,stop); std::signal(SIGTERM,stop);
        peer.start();
        std::cout << "Flight peer waiting for ground; transport=" << transport << std::endl;
        const auto start=std::chrono::steady_clock::now();
        auto last_snapshot=start;
        bool connected=false;
        std::string remote_session;
        while (running && (!seconds || std::chrono::steady_clock::now()-start<std::chrono::seconds(seconds))) {
            ++counter;
            const bool ready=peer.ready();
            if (ready && (!connected || remote_session!=peer.remoteSession())) {
                remote_session=peer.remoteSession();
                log("Ground registration ready: " + peer.remoteNode());
                { std::lock_guard lock(mutex); protocol.argus().publishTelemetry({.owner_node="FLIGHT_COMPUTER", .event=event}); }
                try {
                    DARTWIC::ChannelUpsert command{peer.remoteNode(),"rocket_sim_flight_check","value",1};
                    protocol.channels().upsert(std::move(command));
                    log("Flight to ground command acknowledged: rocket_sim_flight_check=1");
                } catch(const std::exception& error) { log(std::string("Ground command failed: ")+error.what()); }
            }
            if (!ready && connected) std::cout << "Ground disconnected; pending commands fail and telemetry is best effort" << std::endl;
            connected=ready;
            const auto now=std::chrono::steady_clock::now();
            if (ready && std::chrono::duration<double>(now-last_snapshot).count()>=1/rate.load()) {
                TEMPEST::Value::Object records;
                for (const auto& record:snapshots()) records[record.channel]=record.channel_data;
                peer.publish("tempest-peer/channels/snapshot",{{"owner_node","FLIGHT_COMPUTER"}, {"channels",std::move(records)}, {"complete",true}});
                {
                    std::lock_guard lock(mutex);
                    const auto payload = DARTWIC::EngineContract::payload(DARTWIC::ArgusEventTelemetry{.owner_node="FLIGHT_COMPUTER", .event=event});
                    auto board_event = payload.at("event").object();
                    // Board snapshots use ARGUS record identity, including node.
                    board_event["node"] = "FLIGHT_COMPUTER";
                    peer.publish("tempest-peer/argus/board-snapshot", {{"owner_node","FLIGHT_COMPUTER"}, {"events",TEMPEST::Value::Array{std::move(board_event)}}});
                }
                last_snapshot=now;
            }
            std::this_thread::sleep_for(50ms); // Acquisition counter advances at 20 Hz; delivery defaults to 5 Hz.
        }
        peer.stop();
    } catch(const std::exception& error) { std::cerr << error.what() << std::endl; return 1; }
}
