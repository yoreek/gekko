#include "devices/display/DisplayDeviceBase.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS DisplayDeviceBase

DisplayDeviceBase::DisplayDeviceBase(PState initialState) : DeviceRuntimeBase(initialState) {}

DisplayDeviceBase::PState DisplayDeviceBase::initialState() {
    return (PState)&DisplayDeviceBase::Idle;
}

IDevicePersistedState* DisplayDeviceBase::persistedStateRuntime() {
    return this;
}

const IDevicePersistedState* DisplayDeviceBase::persistedStateRuntime() const {
    return this;
}

DeviceValidationResult DisplayDeviceBase::loadPersistedState(DeviceScopedDataStore& store) {
    DisplayLayoutStore layoutStore(store);
    DisplayLayoutRecordV1 layout{};
    const DeviceValidationResult result = layoutStore.load(deviceId(), layout);
    if (!result.ok()) {
        if (result.error == DeviceError::MissingRecord) {
            layout_ = {};
        }
        return result;
    }
    layout_ = layout;
    return {};
}

DeviceValidationResult DisplayDeviceBase::savePersistedState(DeviceScopedDataStore& store) const {
    DisplayLayoutStore layoutStore(store);
    if (layout_.pages.empty()) {
        (void)layoutStore.remove(deviceId());
        return {};
    }
    return layoutStore.save(layout_);
}

DeviceValidationResult DisplayDeviceBase::clearPersistedState(DeviceScopedDataStore& store) {
    DisplayLayoutStore layoutStore(store);
    layout_ = {};
    (void)layoutStore.clearDevice(deviceId());
    return {};
}

DeviceValidationResult DisplayDeviceBase::applyPersistedStateUpdate(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0U) {
        layout_ = {};
        return {};
    }

    DisplayLayoutRecordV1 layout{};
    if (!decodeDisplayLayoutBinary(data, size, layout)) {
        return {DeviceError::InvalidConfig, "display layout is invalid"};
    }
    if (layout.deviceId != 0U && layout.deviceId != deviceId()) {
        return {DeviceError::InvalidDeviceId, "device scoped data device id is invalid"};
    }
    layout.deviceId = deviceId();
    layout_ = layout;
    return {};
}

void DisplayDeviceBase::writeDeviceJson(JsonObject output) const {
    writeCommonDeviceJson(output);
    JsonObject config = output["config"].as<JsonObject>();
    if (!config.isNull()) {
        writeDisplayConfigJson(config);
        JsonObject layout = config.createNestedObject("layout");
        writeDisplayLayoutJson(layout_, layout);
    }
}

const DisplayLayoutRecordV1& DisplayDeviceBase::layout() const {
    return layout_;
}

void DisplayDeviceBase::setLayout(const DisplayLayoutRecordV1& layout) {
    layout_ = layout;
}

SM_STATE(DisplayDeviceBase::Idle) {
    status_ = DeviceStatus::Creating;
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(DisplayDeviceBase::Starting) {
    status_ = DeviceStatus::Starting;
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(Disabled);
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    startRequested_ = false;
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(DisplayDeviceBase::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(DisplayDeviceBase::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    reconfigureRequested_ = false;
    status_ = DeviceStatus::Starting;
    SM_GOTO(Starting);
}

SM_STATE(DisplayDeviceBase::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(DisplayDeviceBase::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(DisplayDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
