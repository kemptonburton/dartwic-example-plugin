#include "example_transport.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <iostream>
#include <mutex>
#include <tempest/JsonValue.h>
#include <tempest/Peer.h>
#include <thread>
#include <zmq.hpp>

namespace Example {
// An application-owned format: four-byte big-endian length followed by JSON.
// TEMPEST itself sees only Message/Value and owns request IDs and replies.
struct ExampleTransport::Impl {
  ExampleTransport &owner;
  std::string receive_endpoint, send_endpoint;
  std::mutex mutex;
  std::condition_variable wake;
  std::deque<TEMPEST::Message> outgoing;
  std::atomic<bool> running{false};
  std::atomic<uint64_t> count{0};
  nlohmann::json last = nlohmann::json::object();
  std::thread worker;
  Impl(ExampleTransport &supplied_owner, const nlohmann::json &config)
      : owner(supplied_owner),
        receive_endpoint(config.at("receive_endpoint").get<std::string>()),
        send_endpoint(config.at("send_endpoint").get<std::string>()) {}
  void run(std::promise<void> ready) {
    bool started = false;
    try {
      zmq::context_t context(1);
      zmq::socket_t receive(context, zmq::socket_type::pull),
          send(context, zmq::socket_type::push);
      receive.set(zmq::sockopt::linger, 0);
      send.set(zmq::sockopt::linger, 0);
      send.set(zmq::sockopt::immediate, 1);
      send.set(zmq::sockopt::sndhwm, 256);
      receive.set(zmq::sockopt::rcvhwm, 256);
      receive.bind(receive_endpoint);
      send.connect(send_endpoint);
      started = true;
      ready.set_value();
      std::string incoming;
      while (running) {
        std::deque<TEMPEST::Message> pending;
        {
          std::lock_guard lock(mutex);
          pending.swap(outgoing);
        }
        for (auto &m : pending) {
          if (m.deadline < std::chrono::steady_clock::now())
            continue;
          nlohmann::json record;
          try {
            record = {{"kind", static_cast<int>(m.kind)},
                      {"id", m.request_id},
                      {"name", m.name},
                      {"payload",
                       TEMPEST::Json::fromValue(TEMPEST::Value{m.payload})},
                      {"error", m.error},
                      {"code", m.error_code}};
          } catch (const std::exception &error) {
            throw TEMPEST::Error("Cannot encode custom frame `" + m.name +
                                 "`: " + error.what());
          }
          const auto body = record.dump();
          if (body.size() > 1024 * 1024)
            throw TEMPEST::Error("Example frame exceeds 1 MiB");
          std::string frame(4, '\0');
          for (int i = 0; i < 4; ++i)
            frame[i] = static_cast<char>(body.size() >> ((3 - i) * 8));
          frame += body;
          if (!send.send(zmq::buffer(frame), zmq::send_flags::dontwait)) {
            std::lock_guard lock(mutex);
            if (outgoing.size() < 256)
              outgoing.push_back(std::move(m));
          }
        }
        zmq::message_t frame;
        while (receive.recv(frame, zmq::recv_flags::dontwait)) {
          try {
            incoming += frame.to_string();
            if (incoming.size() > 2 * 1024 * 1024) {
              incoming.clear();
              continue;
            }
            while (incoming.size() >= 4) {
              uint32_t size = 0;
              for (int i = 0; i < 4; ++i)
                size = (size << 8) | static_cast<unsigned char>(incoming[i]);
              if (size == 0 || size > 1024 * 1024) {
                incoming.clear();
                break;
              }
              if (incoming.size() < size + 4)
                break;
              const auto body = incoming.substr(4, size);
              incoming.erase(0, size + 4);
              try {
                auto record = nlohmann::json::parse(body);
                const int kind = record.at("kind").get<int>();
                if (kind < 0 || kind > 2)
                  continue;
                TEMPEST::Message m{
                    static_cast<TEMPEST::Message::Kind>(kind),
                    record.at("id"),
                    record.at("name"),
                    TEMPEST::Json::toValue(record.at("payload")).object(),
                    record.at("error"),
                    record.at("code")};
                {
                  std::lock_guard lock(mutex);
                  last = record;
                }
                ++count;
                owner.receive(std::move(m));
              } catch (...) { /* A malformed frame does not stop the stream. */
              }
            }
          } catch (...) { /* Reject malformed custom frames without stopping the
                             link. */
          }
        }
        std::unique_lock lock(mutex);
        wake.wait_for(lock, std::chrono::milliseconds(2));
      }
    } catch (const std::exception &error) {
      if (!started)
        ready.set_exception(std::current_exception());
      else {
        std::cerr << "Custom flight link stopped: " << error.what()
                  << std::endl;
        owner.setConnected(false,
                           std::string("Custom link failed: ") + error.what());
      }
      running = false;
    } catch (...) {
      if (!started)
        ready.set_exception(std::current_exception());
      else {
        std::cerr << "Custom flight link stopped: unknown error" << std::endl;
        owner.setConnected(false, "Custom link failed with an unknown error");
      }
      running = false;
    }
  }
};
ExampleTransport::ExampleTransport(nlohmann::json config)
    : impl_(std::make_unique<Impl>(*this, config)) {}
ExampleTransport::~ExampleTransport() { stop(); }
void ExampleTransport::start() {
  if (impl_->running.exchange(true))
    throw TEMPEST::Error("Custom transport is already running");
  std::promise<void> ready;
  auto result = ready.get_future();
  impl_->worker = std::thread([this, ready = std::move(ready)]() mutable {
    impl_->run(std::move(ready));
  });
  result.get();
  setConnected(true);
}
void ExampleTransport::send(TEMPEST::Message message) {
  std::lock_guard lock(impl_->mutex);
  if (!impl_->running)
    throw TEMPEST::DisconnectedError("Custom transport is stopped");
  if (impl_->outgoing.size() >= 256)
    throw TEMPEST::QueueFullError("Custom transport queue is full");
  impl_->outgoing.push_back(std::move(message));
  impl_->wake.notify_one();
}
void ExampleTransport::stop() {
  impl_->running = false;
  impl_->wake.notify_all();
  if (impl_->worker.joinable())
    impl_->worker.join();
  std::lock_guard lock(impl_->mutex);
  impl_->outgoing.clear();
}
std::vector<TEMPEST::TransportPath> ExampleTransport::diagnostics() const {
  std::lock_guard lock(impl_->mutex);
  const auto state = impl_->running ? "open" : "closed";
  return {{"example/pull",
           "PULL incoming frames",
           impl_->receive_endpoint,
           state,
           false,
           0,
           0,
           0,
           0,
           {},
           "example/framed-zmq"},
          {"example/push",
           "PUSH outgoing frames",
           impl_->send_endpoint,
           state,
           false,
           0,
           0,
           impl_->outgoing.size(),
           0,
           {},
           "example/framed-zmq"}};
}
uint64_t ExampleTransport::receivedCount() const { return impl_->count; }
nlohmann::json ExampleTransport::lastFrame() const {
  std::lock_guard lock(impl_->mutex);
  return impl_->last;
}
} // namespace Example
