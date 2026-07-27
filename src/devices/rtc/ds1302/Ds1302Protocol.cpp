#include "devices/rtc/ds1302/Ds1302Protocol.h"

#include "time/DateTime.h"

namespace ewfm {

namespace {
// DS1302 requires >=250ns clock high/low and setup/hold time at 5V (needs more headroom at lower
// Vcc); 2us is comfortably safe on an ESP32 running this bit-bang loop from the main tick.
constexpr uint32_t kClockPulseMicros = 2U;

// Clock/calendar register command bytes (bit7=1 start, bit0=0 write / 1 read appended per access).
constexpr uint8_t kRegSeconds = 0x80;
constexpr uint8_t kRegMinutes = 0x82;
constexpr uint8_t kRegHour = 0x84;
constexpr uint8_t kRegDate = 0x86;
constexpr uint8_t kRegMonth = 0x88;
constexpr uint8_t kRegDay = 0x8A;
constexpr uint8_t kRegYear = 0x8C;
constexpr uint8_t kRegWriteProtect = 0x8E;
constexpr uint8_t kReadBit = 0x01U;

uint8_t bcd2bin(uint8_t val) {
    return static_cast<uint8_t>(val - 6 * (val >> 4));
}

uint8_t bin2bcd(uint8_t val) {
    return static_cast<uint8_t>(val + 6 * (val / 10));
}

bool beginTransaction(IDs1302LineDriver& driver, const Ds1302Pins& pins) {
    if (!driver.setRst(pins.rst, false) || !driver.setClk(pins.clk, false)) {
        return false;
    }
    driver.waitMicros(kClockPulseMicros);
    return driver.setRst(pins.rst, true);
}

bool endTransaction(IDs1302LineDriver& driver, const Ds1302Pins& pins) {
    const bool clkOk = driver.setClk(pins.clk, false);
    const bool rstOk = driver.setRst(pins.rst, false);
    return clkOk && rstOk;
}

bool writeByte(IDs1302LineDriver& driver, const Ds1302Pins& pins, uint8_t value) {
    if (!driver.setDataOutput(pins.data)) {
        return false;
    }
    for (uint8_t bit = 0U; bit < 8U; ++bit) {
        if (!driver.writeData(pins.data, (value & 0x01U) != 0U)) {
            return false;
        }
        value = static_cast<uint8_t>(value >> 1U);
        if (!driver.setClk(pins.clk, true)) {
            return false;
        }
        driver.waitMicros(kClockPulseMicros);
        if (!driver.setClk(pins.clk, false)) {
            return false;
        }
        driver.waitMicros(kClockPulseMicros);
    }
    return true;
}

bool readByte(IDs1302LineDriver& driver, const Ds1302Pins& pins, uint8_t& value) {
    value = 0U;
    if (!driver.setDataInput(pins.data)) {
        return false;
    }
    for (uint8_t bit = 0U; bit < 8U; ++bit) {
        bool level = false;
        if (!driver.readData(pins.data, level)) {
            return false;
        }
        value = static_cast<uint8_t>(value >> 1U);
        if (level) {
            value |= 0x80U;
        }
        if (!driver.setClk(pins.clk, true)) {
            return false;
        }
        driver.waitMicros(kClockPulseMicros);
        if (!driver.setClk(pins.clk, false)) {
            return false;
        }
        driver.waitMicros(kClockPulseMicros);
    }
    return true;
}

bool writeRegister(IDs1302LineDriver& driver, const Ds1302Pins& pins, uint8_t command, uint8_t value) {
    if (!beginTransaction(driver, pins)) {
        return false;
    }
    const bool transferOk = writeByte(driver, pins, command) && writeByte(driver, pins, value);
    return endTransaction(driver, pins) && transferOk;
}

bool readRegister(IDs1302LineDriver& driver, const Ds1302Pins& pins, uint8_t command, uint8_t& value) {
    if (!beginTransaction(driver, pins)) {
        return false;
    }
    const bool transferOk = writeByte(driver, pins, static_cast<uint8_t>(command | kReadBit)) && readByte(driver, pins, value);
    return endTransaction(driver, pins) && transferOk;
}
} // namespace

bool ds1302ReadTime(IDs1302LineDriver& driver, const Ds1302Pins& pins, uint32_t& outUtcEpoch) {
    uint8_t secondsRaw = 0U;
    uint8_t minutesRaw = 0U;
    uint8_t hourRaw = 0U;
    uint8_t dateRaw = 0U;
    uint8_t monthRaw = 0U;
    uint8_t yearRaw = 0U;
    if (!readRegister(driver, pins, kRegSeconds, secondsRaw) || !readRegister(driver, pins, kRegMinutes, minutesRaw) ||
        !readRegister(driver, pins, kRegHour, hourRaw) || !readRegister(driver, pins, kRegDate, dateRaw) ||
        !readRegister(driver, pins, kRegMonth, monthRaw) || !readRegister(driver, pins, kRegYear, yearRaw)) {
        return false;
    }

    // Mask off the clock-halt bit (seconds bit7) and the 12/24-hour mode bit (hour bit7, plus the
    // AM/PM bit6 that only applies in 12-hour mode) - writeTime always programs 24-hour mode, but a
    // module pre-configured by another tool could have left these set.
    const uint8_t second = bcd2bin(secondsRaw & 0x7FU);
    const uint8_t minute = bcd2bin(minutesRaw & 0x7FU);
    const uint8_t hour = bcd2bin(hourRaw & 0x3FU);
    const uint8_t day = bcd2bin(dateRaw & 0x3FU);
    const uint8_t month = bcd2bin(monthRaw & 0x1FU);
    const uint16_t year = static_cast<uint16_t>(bcd2bin(yearRaw) + 2000U);

    const DateTime dt(year, month, day, hour, minute, second);
    outUtcEpoch = dt.utcUnixtime();
    return true;
}

bool ds1302WriteTime(IDs1302LineDriver& driver, const Ds1302Pins& pins, uint32_t utcEpoch) {
    const DateTime dt(utcEpoch);
    // Write protect must be cleared before any other register write, or the chip silently ignores
    // it. Seconds max BCD value is 0x59, so bit7 (clock-halt) is naturally 0 here - writing it
    // starts the oscillator without a separate CH-clearing step.
    if (!writeRegister(driver, pins, kRegWriteProtect, 0x00U)) {
        return false;
    }
    return writeRegister(driver, pins, kRegSeconds, bin2bcd(dt.second())) &&
           writeRegister(driver, pins, kRegMinutes, bin2bcd(dt.minute())) && writeRegister(driver, pins, kRegHour, bin2bcd(dt.hour())) &&
           writeRegister(driver, pins, kRegDate, bin2bcd(dt.day())) && writeRegister(driver, pins, kRegMonth, bin2bcd(dt.month())) &&
           writeRegister(driver, pins, kRegDay, bin2bcd(dt.weekday())) &&
           writeRegister(driver, pins, kRegYear, bin2bcd(static_cast<uint8_t>(dt.year() - 2000U)));
}

} // namespace ewfm
