#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <sdkconfig.h>
#endif

#if defined(WITH_BLE_PROVISIONING) && defined(ARDUINO) && !defined(UNIT_TEST) && defined(CONFIG_BT_ENABLED)

extern "C" bool btInUse() {
    return true;
}

#endif
