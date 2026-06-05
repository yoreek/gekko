#include "update/FirmwareUpdate.h"

namespace ewfm {

FirmwareUpdateError FirmwareUpdatePolicy::validate(size_t imageBytes, size_t metadataBytes) const {
    if (!enabled) {
        return FirmwareUpdateError::Disabled;
    }
    if (metadataBytes > maxMetadataBytes) {
        return FirmwareUpdateError::MetadataTooLarge;
    }
    if (imageBytes == 0) {
        return FirmwareUpdateError::EmptyImage;
    }
    if (availableBytes > 0 && imageBytes > availableBytes) {
        return FirmwareUpdateError::ImageTooLarge;
    }
    return FirmwareUpdateError::None;
}

} // namespace ewfm
