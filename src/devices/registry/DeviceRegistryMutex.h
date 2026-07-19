#pragma once

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

namespace ewfm {

// DeviceRegistry is called both from the main loop task (App::tick() -> tickFastLoop/tick100ms/tick1s/tick)
// and directly from portal HTTP handlers, which ESPAsyncWebServer/AsyncTCP dispatch on their own
// "async_tcp" FreeRTOS task. Recursive because public DeviceRegistry methods call each other
// (e.g. command() -> create()/remove()/setDeps(), tick() -> flushNow()) on the same thread.
class DeviceRegistryMutex {
public:
    DeviceRegistryMutex() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
        handle_ = xSemaphoreCreateRecursiveMutex();
#endif
    }
    ~DeviceRegistryMutex() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
        if (handle_ != nullptr) {
            vSemaphoreDelete(handle_);
        }
#endif
    }
    DeviceRegistryMutex(const DeviceRegistryMutex&) = delete;
    DeviceRegistryMutex& operator=(const DeviceRegistryMutex&) = delete;

    void lock() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
        if (handle_ != nullptr) {
            xSemaphoreTakeRecursive(handle_, portMAX_DELAY);
        }
#endif
    }
    void unlock() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
        if (handle_ != nullptr) {
            xSemaphoreGiveRecursive(handle_);
        }
#endif
    }

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    SemaphoreHandle_t handle_{nullptr};
#endif
};

class DeviceRegistryLockGuard {
public:
    explicit DeviceRegistryLockGuard(const DeviceRegistryMutex& mutex) : mutex_(mutex) {
        mutex_.lock();
    }
    ~DeviceRegistryLockGuard() {
        mutex_.unlock();
    }
    DeviceRegistryLockGuard(const DeviceRegistryLockGuard&) = delete;
    DeviceRegistryLockGuard& operator=(const DeviceRegistryLockGuard&) = delete;

private:
    const DeviceRegistryMutex& mutex_;
};

} // namespace ewfm
