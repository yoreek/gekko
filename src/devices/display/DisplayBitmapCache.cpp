#include "devices/display/DisplayBitmapCache.h"

#include "platform/LittleFsBlobStore.h"

#include <utility>

namespace ewfm {

const std::vector<uint8_t>* ensureDisplayBitmapBytes(const DisplayLayoutWidgetV1& widget) {
    if (widget.cachedImageBytes.has_value()) {
        return &(*widget.cachedImageBytes);
    }
    if (widget.imageKey[0] == '\0') {
        return nullptr;
    }
    LittleFsBlobStore* store = defaultBlobStore();
    if (store == nullptr) {
        return nullptr;
    }
    std::vector<uint8_t> buffer(kDisplayLayoutBitmapDataCapacity);
    size_t length = 0U;
    if (!store->get(widget.imageKey, buffer.data(), buffer.size(), length)) {
        return nullptr;
    }
    buffer.resize(length);
    widget.cachedImageBytes = std::move(buffer);
    return &(*widget.cachedImageBytes);
}

} // namespace ewfm
