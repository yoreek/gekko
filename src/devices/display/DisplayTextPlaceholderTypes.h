#pragma once

#include "devices/core/DeviceTypes.h"
#include "metrics/MetricTypes.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ewfm {

struct DisplayTextPlaceholderSegment {
    MetricNamespace ns{MetricNamespace::Device};
    DeviceId sourceId{0};
    int32_t metricId{0};
};

enum class DisplayTextFilterKind : uint8_t {
    None = 0,
    Text = 1,
    Upper = 2,
    Lower = 3,
    Trim = 4,
};

class MetricValueResolver;
struct MetricValue;

class IDisplayTextValueSource {
public:
    virtual ~IDisplayTextValueSource() = default;

    virtual bool get(const MetricValueResolver& resolver, MetricValue& value) const = 0;
};

using DisplayTextValueSourcePtr = std::shared_ptr<const IDisplayTextValueSource>;

struct DisplayTextLiteralSpan {
    uint16_t offset{0};
    uint16_t length{0};
};

struct DisplayTextSegment {
    bool placeholder{false};
    bool valid{true};
    DisplayTextLiteralSpan literal{};
    DisplayTextPlaceholderSegment reference{};
    DisplayTextFilterKind filter{DisplayTextFilterKind::None};
    DisplayTextValueSourcePtr source{};
};

struct DisplayTextCompiledWidget {
    uint32_t sourceHash{0};
    std::vector<DisplayTextSegment> segments{};
};

enum class DisplayTextCompileStatus : uint8_t {
    Ok = 0,
    InvalidWidget = 1,
    MalformedSyntax = 2,
    UnknownNamespace = 3,
    MissingDeviceId = 4,
    UnknownMetric = 5,
};

struct DisplayTextCompileResult {
    DisplayTextCompileStatus status{DisplayTextCompileStatus::Ok};
    DisplayTextCompiledWidget compiled{};

    bool ok() const {
        return status == DisplayTextCompileStatus::Ok;
    }
};

} // namespace ewfm
