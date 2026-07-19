#pragma once

#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/DisplayTextPlaceholderTypes.h"

#include <string_view>

namespace ewfm {

class MetricValueResolver;
struct DisplayTextEvaluationResult;
class DeviceRegistry;

DisplayTextCompileResult compileDisplayTextWidget(std::string_view text);
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
