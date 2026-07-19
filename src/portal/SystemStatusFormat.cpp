#include "portal/SystemStatusFormat.h"

namespace ewfm {

const char* resetReasonToString(const int espResetReason) {
    switch (espResetReason) {
    case 1:
        return "poweron";
    case 2:
        return "external";
    case 3:
        return "software";
    case 4:
        return "panic";
    case 5:
        return "interruptWatchdog";
    case 6:
        return "taskWatchdog";
    case 7:
        return "otherWatchdog";
    case 8:
        return "deepsleep";
    case 9:
        return "brownout";
    case 10:
        return "sdio";
    default:
        return "unknown";
    }
}

const char* partitionTypeToString(const int type) {
    switch (type) {
    case 0x00:
        return "app";
    case 0x01:
        return "data";
    default:
        return "other";
    }
}

const char* partitionSubtypeToString(const int type, const int subtype) {
    if (type == 0x00) {
        if (subtype == 0x00) {
            return "factory";
        }
        if (subtype >= 0x10 && subtype <= 0x1F) {
            return "ota";
        }
        if (subtype == 0x20) {
            return "test";
        }
        return "other";
    }
    if (type == 0x01) {
        switch (subtype) {
        case 0x00:
            return "otadata";
        case 0x01:
            return "phy";
        case 0x02:
            return "nvs";
        case 0x03:
            return "coredump";
        case 0x04:
            return "nvsKeys";
        case 0x05:
            return "efuse";
        case 0x81:
            return "fat";
        case 0x82:
            return "spiffs";
        default:
            return "other";
        }
    }
    return "other";
}

} // namespace ewfm
