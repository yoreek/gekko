#include "devices/display/DisplayDeviceBase.h"

#include "devices/display/DisplayLayoutValidator.h"

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
            invalidateDisplayRender();
        }
        return result;
    }
    const DeviceValidationResult layoutResult = validateDisplayLayout(layout, displayProfile());
    if (!layoutResult.ok()) {
        return layoutResult;
    }
    layout_ = layout;
    invalidateDisplayRender();
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
    invalidateDisplayRender();
    (void)layoutStore.clearDevice(deviceId());
    return {};
}

DeviceValidationResult DisplayDeviceBase::applyPersistedStateUpdate(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0U) {
        layout_ = {};
        invalidateDisplayRender();
        return {};
    }

    DisplayLayoutRecordV1 layout{};
    if (!decodeDisplayLayoutBinary(data, size, layout)) {
        return {DeviceError::InvalidConfig, "display layout is invalid"};
    }
    if (layout.deviceId != 0U && layout.deviceId != deviceId()) {
        return {DeviceError::InvalidDeviceId, "device scoped data device id is invalid"};
    }
    const DeviceValidationResult layoutResult = validateDisplayLayout(layout, displayProfile());
    if (!layoutResult.ok()) {
        return layoutResult;
    }
    layout.deviceId = deviceId();
    layout_ = layout;
    invalidateDisplayRender();
    return {};
}
const DisplayLayoutRecordV1& DisplayDeviceBase::layout() const {
    return layout_;
}

void DisplayDeviceBase::setLayout(const DisplayLayoutRecordV1& layout) {
    if (!layout.pages.empty() && !validateDisplayLayout(layout, displayProfile()).ok()) {
        return;
    }
    layout_ = layout;
    invalidateDisplayRender();
}

DisplayLayoutProfile DisplayDeviceBase::displayProfile() const {
    return defaultDisplayLayoutProfile();
}

DisplayDeviceBase* DisplayDeviceBase::displayRuntime() {
    return this;
}

const DisplayDeviceBase* DisplayDeviceBase::displayRuntime() const {
    return this;
}

bool DisplayDeviceBase::renderDisplay(const MetricValueResolver& resolver, const uint32_t now) {
    if (status_ != DeviceStatus::Ready) {
        return false;
    }
    if (layout_.pages.empty()) {
        if (emptyLayoutCleared_) {
            return false;
        }
        if (!clearDisplay(layout_.backgroundColor)) {
            return false;
        }
        emptyLayoutCleared_ = true;
        onDisplayFrameRendered({});
        return true;
    }
    emptyLayoutCleared_ = false;
    const DisplayLayoutRenderResult result = renderDisplayFrame(resolver, now);
    if (result.rendered) {
        onDisplayFrameRendered(result);
    }
    return result.rendered;
}

bool DisplayDeviceBase::initializeDisplayHardware(uint32_t now) {
    (void)now;
    return true;
}

void DisplayDeviceBase::releaseDisplayHardware(uint32_t now) {
    (void)now;
}

void DisplayDeviceBase::onDisplayFrameRendered(const DisplayLayoutRenderResult& result) {
    (void)result;
}

void DisplayDeviceBase::invalidateDisplayRender() {
    renderSession_.invalidate();
    emptyLayoutCleared_ = false;
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
    if (!initializeDisplayHardware(uptime())) {
        status_ = DeviceStatus::Faulted;
        requestFault();
        SM_GOTO(Faulted);
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
    releaseDisplayHardware(uptime());
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
    releaseDisplayHardware(uptime());
}

SM_STATE(DisplayDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    releaseDisplayHardware(uptime());
    setDeleted();
}

} // namespace ewfm
