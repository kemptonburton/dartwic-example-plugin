#pragma once

#include <mutex>

#ifdef __linux__
#include <exception>
#include <pthread.h>
#include <system_error>
#endif

namespace Example {

// The read and write tasks share one device state. Linux uses priority
// inheritance so a lower-priority writer cannot indefinitely block a
// real-time reader behind an ordinary mutex owner.
class ExampleDeviceMutex {
public:
    ExampleDeviceMutex() {
#ifdef __linux__
        pthread_mutexattr_t attributes;
        int result = pthread_mutexattr_init(&attributes);
        if (result != 0) throw std::system_error(result, std::generic_category(), "pthread_mutexattr_init");
        result = pthread_mutexattr_setprotocol(&attributes, PTHREAD_PRIO_INHERIT);
        if (result != 0) {
            pthread_mutexattr_destroy(&attributes);
            throw std::system_error(result, std::generic_category(), "Example device requires priority inheritance");
        }
        result = pthread_mutex_init(&mutex_, &attributes);
        pthread_mutexattr_destroy(&attributes);
        if (result != 0) throw std::system_error(result, std::generic_category(), "pthread_mutex_init");
#endif
    }

    ~ExampleDeviceMutex() {
#ifdef __linux__
        if (pthread_mutex_destroy(&mutex_) != 0) std::terminate();
#endif
    }

    ExampleDeviceMutex(const ExampleDeviceMutex&) = delete;
    ExampleDeviceMutex& operator=(const ExampleDeviceMutex&) = delete;

    void lock() {
#ifdef __linux__
        const int result = pthread_mutex_lock(&mutex_);
        if (result != 0) throw std::system_error(result, std::generic_category(), "pthread_mutex_lock");
#else
        mutex_.lock();
#endif
    }

    void unlock() noexcept {
#ifdef __linux__
        if (pthread_mutex_unlock(&mutex_) != 0) std::terminate();
#else
        mutex_.unlock();
#endif
    }

private:
#ifdef __linux__
    pthread_mutex_t mutex_{};
#else
    std::mutex mutex_;
#endif
};

} // namespace Example
