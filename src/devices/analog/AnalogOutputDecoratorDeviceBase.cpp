#include "devices/analog/AnalogOutputDecoratorDeviceBase.h"

#include "devices/analog/AnalogOutputDeviceConfig.h"

#include <ArduinoJson.h>
#include <string_view>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS AnalogOutputDecoratorDeviceBase

AnalogOutputDecoratorDeviceBase::AnalogOutputDecoratorDeviceBase() : DeviceRuntimeBase((PState)&AnalogOutputDecoratorDeviceBase::Idle) {}

void AnalogOutputDecoratorDeviceBase::setDependencyRuntime(const DeviceRole role, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntime(role, dependencyRuntime);
    refreshTarget();
}

void AnalogOutputDecoratorDeviceBase::setDependencyRuntimeAt(const uint8_t index, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntimeAt(index, dependencyRuntime);
    refreshTarget();
}

void AnalogOutputDecoratorDeviceBase::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
    refreshTarget();
}

uint16_t AnalogOutputDecoratorDeviceBase::currentOutputState() const {
    return targetOutput_ != nullptr ? targetOutput_->currentOutputState() : 0U;
}

bool AnalogOutputDecoratorDeviceBase::handleCommand(const DeviceCommand& command) {
    uint16_t state = 0U;
    return parseOutputCommand(command, state) && status() == DeviceStatus::Ready && requestOutputState(state, uptime());
}

IAnalogOutputRuntime* AnalogOutputDecoratorDeviceBase::targetOutput() const {
    return targetOutput_;
}

bool AnalogOutputDecoratorDeviceBase::forwardOutputState(const uint16_t state, const uint32_t now) {
    return targetOutput_ != nullptr && state <= kAnalogOutputLevelMax && targetOutput_->requestOutputState(state, now);
}

void AnalogOutputDecoratorDeviceBase::onTargetAttached(uint32_t now) {
    (void)now;
}

void AnalogOutputDecoratorDeviceBase::refreshTarget() {
    IDeviceRuntime* dependency = dependencyRuntime(DeviceRole::AnalogOutput);
    targetOutput_ = dependency != nullptr ? const_cast<IAnalogOutputRuntime*>(dependency->analogOutputRuntime()) : nullptr;
}

bool AnalogOutputDecoratorDeviceBase::targetReady() const {
    const IDeviceRuntime* dependency = dependencyRuntime(DeviceRole::AnalogOutput);
    return dependency != nullptr && dependency->status() == DeviceStatus::Ready && targetOutput_ != nullptr;
}

bool AnalogOutputDecoratorDeviceBase::parseOutputCommand(const DeviceCommand& command, uint16_t& state) const {
    if (command.type != DeviceCommandType::SetOutput && command.type != DeviceCommandType::Custom) {
        return false;
    }
    StaticJsonDocument<96> doc;
    const std::string_view payload = command.payload.view();
    if (deserializeJson(doc, payload.data(), payload.size())) {
        return false;
    }
    const char* error = nullptr;
    return OutputDeviceValueCodec<uint16_t>::parseJson(doc.as<JsonVariantConst>(), state, error);
}

SM_STATE(AnalogOutputDecoratorDeviceBase::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || baseConfig().enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(AnalogOutputDecoratorDeviceBase::Starting) {
    status_ = DeviceStatus::Starting;
    refreshTarget();
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || baseConfig().enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (!targetReady()) {
        SM_GOTO(DependencyBlocked);
    }
    clearStartRequested();
    onTargetAttached(uptime());
    SM_GOTO(Ready);
}

SM_STATE(AnalogOutputDecoratorDeviceBase::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    refreshTarget();
    clearReconfigureRequested();
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || baseConfig().enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (!targetReady()) {
        SM_GOTO(DependencyBlocked);
    }
    onTargetAttached(uptime());
    SM_GOTO(Ready);
}

SM_STATE(AnalogOutputDecoratorDeviceBase::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || baseConfig().enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        SM_GOTO(Reconfiguring);
    }
    if (!targetReady()) {
        SM_GOTO(DependencyBlocked);
    }
    onReadyTick(uptime());
}

SM_STATE(AnalogOutputDecoratorDeviceBase::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    refreshTarget();
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || baseConfig().enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (targetReady()) {
        onTargetAttached(uptime());
        clearStartRequested();
        clearReconfigureRequested();
        SM_GOTO(Ready);
    }
}

SM_STATE(AnalogOutputDecoratorDeviceBase::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (!disableRequested_ && baseConfig().enabled != 0U) {
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(AnalogOutputDecoratorDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
