#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"

namespace ewfm {

// Shared by every RTC-role REST adapter (rtc_ds3231, rtc_ds1302, ...): only one registered device
// may have useForSystemTimeSync set, since RtcSyncCoordinator::findActiveRtc() picks whichever
// Ready device with DeviceRole::RealTimeClock has that flag and cannot arbitrate between several.
// The check is generic over IRealTimeClockRuntime, so it applies across RTC types, not just within
// one - a DS3231 flagged active blocks a DS1302 from also requesting it, and vice versa.
DeviceValidationResult validateAtMostOneActiveRtcSync(const DeviceRegistry& registry, const IDeviceRuntime* self, bool requestedActive);

} // namespace ewfm
