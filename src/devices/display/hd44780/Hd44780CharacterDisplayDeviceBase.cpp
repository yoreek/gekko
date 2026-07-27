#include "devices/display/hd44780/Hd44780CharacterDisplayDeviceBase.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#endif

#include <cstdio>
#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Hd44780CharacterDisplayDeviceBase

namespace {
void hardwareSettleMicros(uint32_t microseconds) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    delayMicroseconds(microseconds);
#else
    (void)microseconds;
#endif
}

void hardwareSettleMillis(uint32_t milliseconds) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    delay(milliseconds);
#else
    (void)milliseconds;
#endif
}
} // namespace

Hd44780CharacterDisplayDeviceBase::Hd44780CharacterDisplayDeviceBase(PState initialState, uint8_t columns, uint8_t rows)
    : DisplayDeviceBase(initialState), columns_(columns), rows_(rows), renderSurface_(columns, rows) {}

Hd44780CharacterDisplayDeviceBase::PState Hd44780CharacterDisplayDeviceBase::initialState() {
    return (PState)&Hd44780CharacterDisplayDeviceBase::Idle;
}

uint8_t Hd44780CharacterDisplayDeviceBase::dependencySlots(uint8_t* out, uint8_t maxOut) const {
    return hd44780ConfigChannels(channelConfig(), out, maxOut);
}

DisplayLayoutProfile Hd44780CharacterDisplayDeviceBase::displayProfile() const {
    return characterCellDisplayLayoutProfile(columns_, rows_, 0x01U);
}

bool Hd44780CharacterDisplayDeviceBase::renderText(const MetricValueResolver& resolver, uint32_t now) {
    return renderDisplay(resolver, now);
}

const char* Hd44780CharacterDisplayDeviceBase::renderedLine(const uint8_t row) const {
    if (!hasRenderedOnce_ || row >= rows_) {
        return "";
    }
    return lastLines_[row];
}

void Hd44780CharacterDisplayDeviceBase::resetRenderedLines() {
    hasRenderedOnce_ = false;
    for (uint8_t row = 0U; row < kHd44780MaxRows; ++row) {
        lastLines_[row][0] = '\0';
    }
}

ISwitchOutputRuntime* Hd44780CharacterDisplayDeviceBase::dependencySwitch(const uint8_t slot) const {
    const IDeviceRuntime* dependency = dependencyRuntimeAt(slot);
    if (dependency == nullptr) {
        return nullptr;
    }
    return const_cast<ISwitchOutputRuntime*>(dependency->switchOutputRuntime());
}

bool Hd44780CharacterDisplayDeviceBase::writeSwitchSlot(const uint8_t slot, const bool on, const uint32_t now) const {
    ISwitchOutputRuntime* switchRuntime = dependencySwitch(slot);
    return switchRuntime != nullptr && switchRuntime->requestOutputState(on, now);
}

bool Hd44780CharacterDisplayDeviceBase::writeNibble(const uint8_t rsSlot, const uint8_t dataSlot0, const uint8_t dataSlot1,
                                                    const uint8_t dataSlot2, const uint8_t dataSlot3, const uint8_t eSlot,
                                                    const uint8_t nibble, const bool rs, const uint32_t now) const {
    bool ok = writeSwitchSlot(rsSlot, rs, now);
    ok = writeSwitchSlot(dataSlot0, (nibble & 0x1U) != 0U, now) && ok;
    ok = writeSwitchSlot(dataSlot1, (nibble & 0x2U) != 0U, now) && ok;
    ok = writeSwitchSlot(dataSlot2, (nibble & 0x4U) != 0U, now) && ok;
    ok = writeSwitchSlot(dataSlot3, (nibble & 0x8U) != 0U, now) && ok;
    ok = writeSwitchSlot(eSlot, true, now) && ok;
    hardwareSettleMicros(1U);
    ok = writeSwitchSlot(eSlot, false, now) && ok;
    hardwareSettleMicros(50U);
    return ok;
}

bool Hd44780CharacterDisplayDeviceBase::writeByte(const uint8_t rsSlot, const uint8_t dataSlot0, const uint8_t dataSlot1,
                                                  const uint8_t dataSlot2, const uint8_t dataSlot3, const uint8_t eSlot,
                                                  const uint8_t value, const bool rs, const uint32_t now) const {
    const bool highOk = writeNibble(rsSlot, dataSlot0, dataSlot1, dataSlot2, dataSlot3, eSlot, static_cast<uint8_t>(value >> 4U), rs, now);
    const bool lowOk = writeNibble(rsSlot, dataSlot0, dataSlot1, dataSlot2, dataSlot3, eSlot, static_cast<uint8_t>(value & 0x0FU), rs, now);
    return highOk && lowOk;
}

bool Hd44780CharacterDisplayDeviceBase::runInitSequence(const uint32_t now) {
    const Hd44780ChannelConfigV1& channels = channelConfig();
    if (dependencySwitch(channels.rsChannel) == nullptr || dependencySwitch(channels.eChannel) == nullptr ||
        dependencySwitch(channels.d4Channel) == nullptr || dependencySwitch(channels.d5Channel) == nullptr ||
        dependencySwitch(channels.d6Channel) == nullptr || dependencySwitch(channels.d7Channel) == nullptr) {
        return false;
    }

    bool ok = true;
    const uint8_t backlightChannel = channelConfig().backlightChannel;
    if (backlightChannel != kHd44780ChannelUnset) {
        ok = writeSwitchSlot(backlightChannel, true, now) && ok;
    }

    ok = writeNibble(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel, channels.eChannel,
                     0x03U, false, now) &&
         ok;
    hardwareSettleMillis(5U);
    ok = writeNibble(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel, channels.eChannel,
                     0x03U, false, now) &&
         ok;
    hardwareSettleMicros(150U);
    ok = writeNibble(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel, channels.eChannel,
                     0x03U, false, now) &&
         ok;
    hardwareSettleMicros(150U);
    ok = writeNibble(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel, channels.eChannel,
                     0x02U, false, now) &&
         ok;
    hardwareSettleMicros(150U);

    ok = writeByte(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel, channels.eChannel,
                   0x28U, false, now) &&
         ok;
    ok = writeByte(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel, channels.eChannel,
                   0x0CU, false, now) &&
         ok;
    ok = writeByte(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel, channels.eChannel,
                   0x06U, false, now) &&
         ok;
    ok = writeByte(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel, channels.eChannel,
                   0x01U, false, now) &&
         ok;
    hardwareSettleMillis(2U);
    return ok;
}

uint8_t Hd44780CharacterDisplayDeviceBase::rowAddress(const uint8_t row) const {
    const uint8_t base = (row % 2U != 0U) ? 0x40U : 0x00U;
    const uint8_t halfOffset = (row >= 2U) ? columns_ : 0U;
    return static_cast<uint8_t>(base + halfOffset);
}

bool Hd44780CharacterDisplayDeviceBase::writeLine(const uint8_t rsSlot, const uint8_t dataSlot0, const uint8_t dataSlot1,
                                                  const uint8_t dataSlot2, const uint8_t dataSlot3, const uint8_t eSlot, const uint8_t row,
                                                  const char* text, const uint32_t now) const {
    bool ok =
        writeByte(rsSlot, dataSlot0, dataSlot1, dataSlot2, dataSlot3, eSlot, static_cast<uint8_t>(0x80U | rowAddress(row)), false, now);
    for (uint8_t i = 0; i < columns_; ++i) {
        ok = writeByte(rsSlot, dataSlot0, dataSlot1, dataSlot2, dataSlot3, eSlot, static_cast<uint8_t>(text[i]), true, now) && ok;
    }
    return ok;
}

bool Hd44780CharacterDisplayDeviceBase::clearDisplay(const uint16_t color) {
    renderSurface_.clear(color);
    return true;
}

DisplayLayoutRenderResult Hd44780CharacterDisplayDeviceBase::renderDisplayFrame(const MetricValueResolver& resolver, const uint32_t now) {
    CharacterDisplayLayoutRenderer renderer(renderSurface_);
    return renderSession_.render(layout_, resolver, renderer, now);
}

bool Hd44780CharacterDisplayDeviceBase::initializeDisplayHardware(uint32_t now) {
    resetRenderedLines();
    return runInitSequence(now);
}

void Hd44780CharacterDisplayDeviceBase::releaseDisplayHardware(uint32_t now) {
    (void)now;
}

void Hd44780CharacterDisplayDeviceBase::onDisplayFrameRendered(const DisplayLayoutRenderResult&) {
    const Hd44780ChannelConfigV1& channels = channelConfig();
    if (dependencySwitch(channels.rsChannel) == nullptr || dependencySwitch(channels.eChannel) == nullptr ||
        dependencySwitch(channels.d4Channel) == nullptr || dependencySwitch(channels.d5Channel) == nullptr ||
        dependencySwitch(channels.d6Channel) == nullptr || dependencySwitch(channels.d7Channel) == nullptr) {
        return;
    }

    bool changed = false;
    if (layout_.pages.empty()) {
        for (uint8_t row = 0U; row < rows_; ++row) {
            char blank[kHd44780MaxColumns + 1U];
            for (uint8_t column = 0U; column < columns_; ++column) {
                blank[column] = ' ';
            }
            blank[columns_] = '\0';
            if (!hasRenderedOnce_ || std::strcmp(blank, lastLines_[row]) != 0) {
                if (!writeLine(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel,
                               channels.eChannel, row, blank, uptime())) {
                    return;
                }
                std::memcpy(lastLines_[row], blank, columns_ + 1U);
                changed = true;
            }
        }
        hasRenderedOnce_ = true;
        (void)changed;
        return;
    }

    for (uint8_t row = 0U; row < rows_; ++row) {
        const char* resolved = renderSurface_.line(row);
        if (!hasRenderedOnce_ || std::strcmp(resolved, lastLines_[row]) != 0) {
            if (!writeLine(channels.rsChannel, channels.d4Channel, channels.d5Channel, channels.d6Channel, channels.d7Channel,
                           channels.eChannel, row, resolved, uptime())) {
                return;
            }
            std::memcpy(lastLines_[row], resolved, columns_ + 1U);
            changed = true;
        }
    }
    hasRenderedOnce_ = true;
    (void)changed;
}

SM_STATE(Hd44780CharacterDisplayDeviceBase::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(Hd44780CharacterDisplayDeviceBase::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        return;
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!initializeDisplayHardware(uptime())) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
    startRequested_ = false;
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(Hd44780CharacterDisplayDeviceBase::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependenciesReady()) {
        SM_GOTO(Starting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Hd44780CharacterDisplayDeviceBase::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    reconfigureRequested_ = false;
    SM_GOTO(Starting);
}

SM_STATE(Hd44780CharacterDisplayDeviceBase::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Hd44780CharacterDisplayDeviceBase::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
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

SM_STATE(Hd44780CharacterDisplayDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
