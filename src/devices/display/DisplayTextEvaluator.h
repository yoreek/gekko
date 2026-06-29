#pragma once

#include "devices/display/DisplayLayoutStore.h"
#include "metrics/MetricValueResolver.h"

#include <cstddef>

namespace ewfm {

constexpr size_t kDisplayEvaluatedTextCapacity = 96;

enum class DisplayTextEvaluationStatus : uint8_t {
    Static = 0,
    Resolved = 1,
    MissingMetric = 2,
    InvalidWidget = 3,
    InvalidPlaceholder = 4,
    Truncated = 5,
};

struct DisplayTextEvaluationResult {
    DisplayTextEvaluationStatus status{DisplayTextEvaluationStatus::Static};
    bool dynamic{false};
    bool available{true};
    char text[kDisplayEvaluatedTextCapacity]{};
};

bool evaluateDisplayTextWidget(const DisplayLayoutWidgetV1& widget, const MetricValueResolver& resolver,
                               DisplayTextEvaluationResult& result);

} // namespace ewfm
