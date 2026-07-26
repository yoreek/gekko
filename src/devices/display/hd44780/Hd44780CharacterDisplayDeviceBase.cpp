#include "devices/display/hd44780/Hd44780CharacterDisplayDeviceBase.h"

#include "devices/display/DisplayTextEvaluator.h"
#include "devices/display/DisplayTextPlaceholderAst.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#endif

#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Hd44780CharacterDisplayDeviceBase

namespace {
// HD44780 timing needs microsecond/millisecond hardware settle waits (enable pulse width, command
// execution time) that have nothing to do with cooperative tick scheduling -- every Arduino HD44780
// library does the same. No-op under native unit tests, which exercise the protocol logic against a
// fake IPortExpanderRuntime and don't care about real timing.
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
    : DeviceRuntimeBase(initialState), columns_(columns), rows_(rows) {}

Hd44780CharacterDisplayDeviceBase::PState Hd44780CharacterDisplayDeviceBase::initialState() {
    return (PState)&Hd44780CharacterDisplayDeviceBase::Idle;
}

uint8_t Hd44780CharacterDisplayDeviceBase::expanderChannels(uint8_t* out, uint8_t maxOut) const {
    return hd44780ConfigChannels(channelConfig(), out, maxOut);
}

CharacterDisplayRuntimeBase* Hd44780CharacterDisplayDeviceBase::characterDisplayRuntime() {
    return this;
}

const char* Hd44780CharacterDisplayDeviceBase::renderedLine(uint8_t row) const {
    if (!hasRenderedOnce_ || row >= rows_) {
        return "";
    }
    return lastLines_[row];
}

void Hd44780CharacterDisplayDeviceBase::resetRenderedLines() {
    hasRenderedOnce_ = false;
}

IPortExpanderRuntime* Hd44780CharacterDisplayDeviceBase::dependencyExpander() const {
    IDeviceRuntime* dependency = dependencyRuntime(DeviceRole::PortExpander);
    if (dependency == nullptr) {
        return nullptr;
    }
    return const_cast<IPortExpanderRuntime*>(dependency->portExpanderRuntime());
}

bool Hd44780CharacterDisplayDeviceBase::writeNibble(IPortExpanderRuntime& expander, const uint8_t nibble, const bool rs,
                                                    const uint32_t now) const {
    const Hd44780ChannelConfigV1& channels = channelConfig();
    bool ok = expander.requestChannelState(channels.rsChannel, rs, now);
    ok = expander.requestChannelState(channels.d4Channel, (nibble & 0x1U) != 0U, now) && ok;
    ok = expander.requestChannelState(channels.d5Channel, (nibble & 0x2U) != 0U, now) && ok;
    ok = expander.requestChannelState(channels.d6Channel, (nibble & 0x4U) != 0U, now) && ok;
    ok = expander.requestChannelState(channels.d7Channel, (nibble & 0x8U) != 0U, now) && ok;
    ok = expander.requestChannelState(channels.eChannel, true, now) && ok;
    hardwareSettleMicros(1U);
    ok = expander.requestChannelState(channels.eChannel, false, now) && ok;
    hardwareSettleMicros(50U);
    return ok;
}

bool Hd44780CharacterDisplayDeviceBase::writeByte(IPortExpanderRuntime& expander, const uint8_t value, const bool rs,
                                                  const uint32_t now) const {
    const bool highOk = writeNibble(expander, static_cast<uint8_t>(value >> 4U), rs, now);
    const bool lowOk = writeNibble(expander, static_cast<uint8_t>(value & 0x0FU), rs, now);
    return highOk && lowOk;
}

bool Hd44780CharacterDisplayDeviceBase::runInitSequence(const uint32_t now) {
    IPortExpanderRuntime* expander = dependencyExpander();
    if (expander == nullptr) {
        return false;
    }

    bool ok = true;
    const uint8_t backlightChannel = channelConfig().backlightChannel;
    if (backlightChannel != kHd44780ChannelUnset) {
        ok = expander->requestChannelState(backlightChannel, true, now) && ok;
    }

    // HD44780 4-bit initialization by instruction (datasheet "Initializing by Instruction"): force
    // 8-bit mode three times regardless of the controller's current (unknown) state, then switch to
    // 4-bit mode, then the usual function-set/display-on/entry-mode/clear command sequence. The
    // function-set "N" bit (2-or-more lines) is identical for 2-line and 4-line panels -- 4-line
    // mode is entirely a DDRAM-addressing convention (rowAddress()), not a separate command.
    ok = writeNibble(*expander, 0x03U, false, now) && ok;
    hardwareSettleMillis(5U);
    ok = writeNibble(*expander, 0x03U, false, now) && ok;
    hardwareSettleMicros(150U);
    ok = writeNibble(*expander, 0x03U, false, now) && ok;
    hardwareSettleMicros(150U);
    ok = writeNibble(*expander, 0x02U, false, now) && ok;
    hardwareSettleMicros(150U);

    ok = writeByte(*expander, 0x28U, false, now) && ok; // function set: 4-bit, 2+ lines, 5x8 font
    ok = writeByte(*expander, 0x0CU, false, now) && ok; // display on, cursor off, blink off
    ok = writeByte(*expander, 0x06U, false, now) && ok; // entry mode: increment, no shift
    ok = writeByte(*expander, 0x01U, false, now) && ok; // clear display
    hardwareSettleMillis(2U);
    return ok;
}

uint8_t Hd44780CharacterDisplayDeviceBase::rowAddress(uint8_t row) const {
    const uint8_t base = (row % 2U != 0U) ? 0x40U : 0x00U;
    const uint8_t halfOffset = (row >= 2U) ? columns_ : 0U;
    return static_cast<uint8_t>(base + halfOffset);
}

bool Hd44780CharacterDisplayDeviceBase::writeLine(IPortExpanderRuntime& expander, const uint8_t row, const char* text,
                                                  const uint32_t now) const {
    bool ok = writeByte(expander, static_cast<uint8_t>(0x80U | rowAddress(row)), false, now);
    for (uint8_t i = 0; i < columns_; ++i) {
        ok = writeByte(expander, static_cast<uint8_t>(text[i]), true, now) && ok;
    }
    return ok;
}

void Hd44780CharacterDisplayDeviceBase::resolveLine(const char* templateText, const MetricValueResolver& resolver, char* out,
                                                    const uint8_t capacity) const {
    const DisplayTextCompileResult compiled = compileDisplayTextWidget(templateText);
    DisplayTextEvaluationResult evaluated{};
    const bool resolved = compiled.ok() && evaluateDisplayTextWidget(templateText, compiled.compiled, resolver, evaluated);
    const char* source = resolved ? evaluated.text : templateText;

    const uint8_t lineLength = static_cast<uint8_t>(capacity - 1U);
    uint8_t length = 0U;
    while (length < lineLength && source[length] != '\0') {
        out[length] = source[length];
        ++length;
    }
    for (uint8_t i = length; i < lineLength; ++i) {
        out[i] = ' ';
    }
    out[lineLength] = '\0';
}

bool Hd44780CharacterDisplayDeviceBase::renderText(const MetricValueResolver& resolver, const uint32_t now) {
    if (status_ != DeviceStatus::Ready) {
        return false;
    }
    IPortExpanderRuntime* expander = dependencyExpander();
    if (expander == nullptr) {
        return false;
    }

    bool changed = false;
    for (uint8_t row = 0; row < rows_; ++row) {
        char resolved[kHd44780MaxColumns + 1U];
        resolveLine(lineTemplate(row), resolver, resolved, static_cast<uint8_t>(columns_ + 1U));
        if (!hasRenderedOnce_ || std::strcmp(resolved, lastLines_[row]) != 0) {
            if (!writeLine(*expander, row, resolved, now)) {
                return false;
            }
            std::memcpy(lastLines_[row], resolved, columns_ + 1U);
            changed = true;
        }
    }
    hasRenderedOnce_ = true;
    return changed;
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
    if (!runInitSequence(uptime())) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
    resetRenderedLines();
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
