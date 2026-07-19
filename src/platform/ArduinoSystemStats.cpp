#include "platform/ArduinoSystemStats.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Esp.h>
#endif

namespace ewfm {

uint32_t ArduinoSystemStats::freeHeapBytes() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return ESP.getFreeHeap();
#else
    return 0;
#endif
}

uint8_t ArduinoSystemStats::heapFragmentationPercent() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap == 0) {
        return 0;
    }
    const uint32_t largestBlock = ESP.getMaxAllocHeap();
    return static_cast<uint8_t>(100U - (largestBlock * 100U / freeHeap));
#else
    return 0;
#endif
}

uint32_t ArduinoSystemStats::minFreeHeapBytes() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return ESP.getMinFreeHeap();
#else
    return 0;
#endif
}

uint32_t ArduinoSystemStats::largestFreeBlockBytes() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return ESP.getMaxAllocHeap();
#else
    return 0;
#endif
}

} // namespace ewfm
