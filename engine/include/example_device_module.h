#pragma once

#include <modules/BaseModule.h>

#include <mutex>

namespace Example {
    struct MockDeviceData {
        double measurement = 0.0;
        double commanded = 0.0;
        double applied = 0.0;
    };

    class ExampleDeviceModule final : public DARTWIC::Modules::BaseModule {
    public:
        ExampleDeviceModule(nlohmann::json cfg, DARTWIC::API::SDK_API* api)
            : BaseModule(std::move(cfg), api) {}

        // This is deliberately small: it is a stand-in for a device connection and
        // its protocol state. The read and write task types share one instance.
        bool ensureConnected() {
            std::scoped_lock lock(mutex_);
            connected_ = true;
            return connected_;
        }

        MockDeviceData readDevice() {
            std::scoped_lock lock(mutex_);
            ++data_.measurement;
            return data_;
        }

        void writeDevice(const double command) {
            std::scoped_lock lock(mutex_);
            data_.commanded = command;
            data_.applied = command;
        }

    private:
        std::mutex mutex_;
        bool connected_ = false;
        MockDeviceData data_;
    };
}
