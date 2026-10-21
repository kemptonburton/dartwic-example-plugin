#pragma once

#include <tempest/Transport.h>
#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <memory>

namespace Example {

/**
 * Custom transport example. The implementation is kept out of the header so
 * plugin registration does not pull the chosen networking library into every
 * plugin translation unit.
 */
class ExampleTransport final : public TEMPEST::Transport {
public:
    explicit ExampleTransport(nlohmann::json config);
    ~ExampleTransport() override;

    void start(TEMPEST::TransportCallbacks callbacks) override;
    void send(TEMPEST::Message message) override;
    void stop() override;
    std::vector<TEMPEST::TransportPath> diagnostics() const override;

    uint64_t receivedCount() const;
    nlohmann::json lastFrame() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Example
