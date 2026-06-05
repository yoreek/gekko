#pragma once

#include <cstddef>

namespace ewfm {

enum class FirmwareUpdateError {
    None,
    Disabled,
    EmptyImage,
    ImageTooLarge,
    MetadataTooLarge,
    UpdateBeginFailed,
    WriteFailed,
    FinalizeFailed,
};

struct FirmwareUpdatePolicy {
    bool enabled{false};
    size_t availableBytes{0};
    size_t maxMetadataBytes{1024};

    FirmwareUpdateError validate(size_t imageBytes, size_t metadataBytes = 0) const;
};

} // namespace ewfm
