#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/DisplayTextPlaceholderTypes.h"

#include <array>
#include <string_view>

namespace ewfm {

class MetricValueResolver;
struct DisplayTextEvaluationResult;
class DeviceRegistry;

DisplayTextCompileResult compileDisplayTextWidget(std::string_view text);
// Scans `text` for dev.<id>.<key> placeholders and appends each referenced device id, deduped, as
// a MetricSource dependency link into `dependencies`/`dependencyCount`. Returns false (with `error`
// set to `invalidTextError`/`dependencyCountError`) on an invalid placeholder or if appending would
// exceed `dependencies`' capacity. Shared by any device family that resolves placeholder text
// against other devices' metrics, whether or not it uses the pixel-widget/page layout model.
bool collectTextPlaceholderDeviceIds(std::string_view text, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& dependencies,
                                     uint8_t& dependencyCount, const char*& error, const char* invalidTextError,
                                     const char* dependencyCountError);
bool ensureDisplayTextWidgetAst(const DisplayLayoutWidgetV1& widget, const DisplayTextCompiledWidget*& compiled,
                                DisplayTextCompileStatus* status = nullptr);
bool prepareDisplayLayoutTextAst(const DisplayLayoutRecordV1& layout);
bool bindDisplayLayoutTextAst(const DisplayLayoutRecordV1& layout, const DeviceRegistry& registry);
DeviceValidationResult validateDisplayTextWidget(const DisplayLayoutWidgetV1& widget, const DeviceRegistry& registry);
bool evaluateDisplayTextWidget(std::string_view sourceText, const DisplayTextCompiledWidget& compiled, const MetricValueResolver& resolver,
                               DisplayTextEvaluationResult& result);
bool evaluateDisplayTextWidget(const DisplayLayoutWidgetV1& widget, const MetricValueResolver& resolver,
                               DisplayTextEvaluationResult& result);
bool displayTextPlaceholderSupportsRuntime(const IDeviceRuntime& runtime, const DisplayTextPlaceholderSegment& placeholder);
bool displayTextWidgetUsesStructuredPlaceholders(const DisplayLayoutWidgetV1& widget);

} // namespace ewfm
