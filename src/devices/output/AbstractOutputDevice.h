#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/output/OutputDeviceConfig.h"
#include "devices/registry/DeviceRetainedDataStore.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ewfm {

template <typename ValueType> struct OutputDeviceRetainedStateRecord {
    using StoredValueType = typename std::conditional<std::is_same<ValueType, bool>::value, uint8_t, ValueType>::type;

    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0};
    StoredValueType state{};
    uint8_t legacyDisabled{0U};
};

using BoolOutputDeviceRetainedStateRecord = OutputDeviceRetainedStateRecord<bool>;
static_assert(offsetof(BoolOutputDeviceRetainedStateRecord, state) == 8U, "bool output retained state layout changed");
static_assert(sizeof(BoolOutputDeviceRetainedStateRecord) == 12U, "bool output retained state layout changed");

template <typename ValueType, typename RuntimeInterface> class AbstractOutputDevice : public DeviceRuntimeBase, public RuntimeInterface {
public:
    using StateType = typename RuntimeInterface::StateType;
    using ConfigType = OutputDeviceConfigV1<ValueType>;

    StateType currentOutputState() const final {
        return currentState_;
    }

    bool requestOutputState(const StateType state, const uint32_t now) final {
        return requestState(state, now);
    }

    bool handleCommand(const DeviceCommand& command) final {
        StateType requestedState{};
        if (!parseOutputCommand(command, requestedState)) {
            return false;
        }
        return requestOutputState(requestedState, uptime());
    }

    bool retainedStateDirty() const override {
        return retainedStateDirty_;
    }

    DeviceValidationResult saveRetainedState(DeviceRetainedDataStore& store) const override {
        static_assert(std::is_trivially_copyable<StateType>::value, "output state must be trivially copyable");
        if (!shouldRestorePreviousState()) {
            return {};
        }
        OutputDeviceRetainedStateRecord<ValueType> record{};
        record.deviceId = deviceId();
        record.state = static_cast<typename OutputDeviceRetainedStateRecord<ValueType>::StoredValueType>(retainedState_);
        record.legacyDisabled = 0U;
        return store.save(record);
    }

    DeviceValidationResult loadRetainedState(DeviceRetainedDataStore& store) override {
        OutputDeviceRetainedStateRecord<ValueType> record{};
        const DeviceValidationResult result = store.load(deviceId(), record);
        if (!result.ok()) {
            return result;
        }
        StateType loadedState{};
        if (!decodeRetainedState(record, loadedState) || !stateIsValid(loadedState)) {
            return {DeviceError::InvalidConfig, "output retained state is invalid"};
        }
        retainedState_ = loadedState;
        retainedStateAvailable_ = true;
        return {};
    }

    void clearRetainedStateDirty() override {
        retainedStateDirty_ = false;
    }

    void end(uint32_t now) override {
        releaseHardware(now);
    }

protected:
    explicit AbstractOutputDevice(const StateType initialState)
        : DeviceRuntimeBase((PState)&AbstractOutputDevice::Idle), currentState_(initialState), retainedState_(initialState) {}

    const DeviceBaseConfigV1& baseConfig() const final {
        return config();
    }

    virtual const ConfigType& config() const = 0;
    ValueType toPhysicalState(ValueType state) const {
        return config().inverted ? invertedState(state) : state;
    }
    virtual ValueType invertedState(ValueType state) const = 0;
    virtual bool parseOutputCommand(const DeviceCommand& command, StateType& state) const = 0;
    virtual bool stateIsValid(StateType state) const = 0;
    virtual DeviceValidationResult configureHardware(uint32_t now) = 0;
    virtual DeviceValidationResult applyHardwareOutput(ValueType state, uint32_t now) = 0;
    virtual void hardwareOutputApplied(ValueType state) {
        (void)state;
    }
    virtual void releaseHardware(uint32_t now) = 0;

    template <typename ConfigType, typename Decoder>
    static DeviceValidationResult validateOutputConfig(const DeviceConfigBlob& configBlob, Decoder decode) {
        if (configBlob.size() > kMaxDeviceConfigBytes) {
            return {DeviceError::BoundsExceeded, "output config exceeds supported size"};
        }
        ConfigType config{};
        if (!decode(configBlob.data(), configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, "output config is invalid"};
        }
        return config.validate();
    }

    bool applyRetainedState(const StateType state) {
        if (!stateIsValid(state)) {
            return false;
        }
        retainedState_ = state;
        retainedStateAvailable_ = true;
        return true;
    }

private:
    DeviceValidationResult applyHardwareState(const StateType state, const uint32_t now) {
        const ValueType physicalState = toPhysicalState(state);
        const DeviceValidationResult result = applyHardwareOutput(physicalState, now);
        if (result.ok()) {
            hardwareOutputApplied(physicalState);
        }
        return result;
    }

    bool requestState(const StateType requestedState, const uint32_t now) {
        if (status_ != DeviceStatus::Ready || !stateIsValid(requestedState)) {
            return false;
        }
        return applyOutputState(requestedState, now, shouldRestorePreviousState());
    }

    StateType configuredStartupState() const {
        return config().startupState;
    }

    StateType configuredSafeState() const {
        return config().safeState;
    }

    bool shouldRestorePreviousState() const {
        return config().restorePreviousState;
    }

    static bool decodeRetainedState(const OutputDeviceRetainedStateRecord<ValueType>& record, StateType& state) {
        if (record.legacyDisabled > 1U) {
            return false;
        }
        if constexpr (std::is_same<ValueType, bool>::value) {
            if (record.state > 1U) {
                // Legacy switch records stored off/on/disabled directly as values 0/1/2.
                if (record.state == 2U && record.legacyDisabled == 0U) {
                    state = false;
                    return true;
                }
                return false;
            }
            state = record.legacyDisabled == 0U && record.state != 0U;
            return true;
        }
        state = static_cast<ValueType>(record.state);
        return true;
    }

    void Idle() {
        status_ = DeviceStatus::Creating;
        if (startRequested_) {
            transitionTo((PState)&AbstractOutputDevice::Starting, uptime());
        }
    }

    void Starting() {
        status_ = DeviceStatus::Starting;
        if (!dependenciesReady()) {
            status_ = DeviceStatus::DependencyBlocked;
            transitionTo((PState)&AbstractOutputDevice::DependencyBlocked, uptime());
            return;
        }
        if (deleteRequested_) {
            enterDeleting();
            return;
        }
        if (disableRequested_ || !enabled()) {
            enterDisabled();
            return;
        }

        const StateType selectedState = shouldRestorePreviousState() && retainedStateAvailable_ ? retainedState_ : configuredStartupState();
        if (!stateIsValid(selectedState) || !configureHardware(uptime()).ok() || !applyOutputState(selectedState, uptime(), false)) {
            status_ = DeviceStatus::Faulted;
            transitionTo((PState)&AbstractOutputDevice::Faulted, uptime());
            return;
        }
        clearStartRequested();
        status_ = DeviceStatus::Ready;
        transitionTo((PState)&AbstractOutputDevice::Ready, uptime());
    }

    void Ready() {
        status_ = DeviceStatus::Ready;
        if (!dependenciesReady()) {
            releaseHardware(uptime());
            status_ = DeviceStatus::DependencyBlocked;
            transitionTo((PState)&AbstractOutputDevice::DependencyBlocked, uptime());
            return;
        }
        if (deleteRequested_) {
            enterDeleting();
            return;
        }
        if (disableRequested_ || !enabled()) {
            enterDisabled();
            return;
        }
        if (reconfigureRequested_) {
            status_ = DeviceStatus::Reconfiguring;
            transitionTo((PState)&AbstractOutputDevice::Reconfiguring, uptime());
            return;
        }
        if (faultRequested_) {
            status_ = DeviceStatus::Faulted;
            transitionTo((PState)&AbstractOutputDevice::Faulted, uptime());
        }
    }

    void Reconfiguring() {
        status_ = DeviceStatus::Reconfiguring;
        clearReconfigureRequested();
        releaseHardware(uptime());
        if (deleteRequested_) {
            enterDeleting();
            return;
        }
        if (disableRequested_ || !enabled()) {
            enterDisabled();
            return;
        }
        if (!dependenciesReady()) {
            status_ = DeviceStatus::DependencyBlocked;
            transitionTo((PState)&AbstractOutputDevice::DependencyBlocked, uptime());
            return;
        }
        if (!configureHardware(uptime()).ok() || !applyOutputState(currentState_, uptime(), false)) {
            status_ = DeviceStatus::Faulted;
            transitionTo((PState)&AbstractOutputDevice::Faulted, uptime());
            return;
        }
        status_ = DeviceStatus::Ready;
        transitionTo((PState)&AbstractOutputDevice::Ready, uptime());
    }

    void DependencyBlocked() {
        status_ = DeviceStatus::DependencyBlocked;
        if (deleteRequested_) {
            enterDeleting();
            return;
        }
        if (disableRequested_ || !enabled()) {
            enterDisabled();
            return;
        }
        if ((reconfigureRequested_ || startRequested_) && dependenciesReady()) {
            status_ = DeviceStatus::Reconfiguring;
            transitionTo((PState)&AbstractOutputDevice::Reconfiguring, uptime());
        }
    }

    void Disabled() {
        status_ = DeviceStatus::Disabled;
        if (deleteRequested_) {
            enterDeleting();
            return;
        }
        if (reconfigureRequested_ && enabled()) {
            status_ = DeviceStatus::Reconfiguring;
            transitionTo((PState)&AbstractOutputDevice::Reconfiguring, uptime());
        }
    }

    void Faulted() {
        status_ = DeviceStatus::Faulted;
        releaseHardware(uptime());
        if (deleteRequested_) {
            enterDeleting();
            return;
        }
        if (reconfigureRequested_) {
            clearFaultRequested();
            status_ = DeviceStatus::Reconfiguring;
            transitionTo((PState)&AbstractOutputDevice::Reconfiguring, uptime());
        }
    }

    void Deleting() {
        status_ = DeviceStatus::Deleting;
        setDeleted();
    }

    bool applyOutputState(const StateType state, const uint32_t now, const bool markRetainedDirty) {
        if (!stateIsValid(state)) {
            return false;
        }
        if (status_ == DeviceStatus::Ready && state == currentState_) {
            return true;
        }
        const DeviceValidationResult result = applyHardwareState(state, now);
        if (!result.ok()) {
            requestFault();
            return false;
        }
        if (state != currentState_) {
            currentState_ = state;
            markRuntimeStateDirty();
        }
        if (markRetainedDirty) {
            retainedState_ = state;
            retainedStateAvailable_ = true;
            retainedStateDirty_ = true;
        }
        return true;
    }

    void enterDisabled() {
        const StateType disabledState = configuredSafeState();
        if (stateIsValid(disabledState)) {
            (void)applyHardwareState(disabledState, uptime());
            if (disabledState != currentState_) {
                currentState_ = disabledState;
                markRuntimeStateDirty();
            }
        }
        releaseHardware(uptime());
        status_ = DeviceStatus::Disabled;
        transitionTo((PState)&AbstractOutputDevice::Disabled, uptime());
    }

    void enterDeleting() {
        releaseHardware(uptime());
        status_ = DeviceStatus::Deleting;
        setDeleted();
        transitionTo((PState)&AbstractOutputDevice::Deleting, uptime());
    }

    StateType currentState_{};
    StateType retainedState_{};
    bool retainedStateAvailable_{false};
    bool retainedStateDirty_{false};
};

} // namespace ewfm
