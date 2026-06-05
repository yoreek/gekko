#if defined(ARDUINO) && !defined(UNIT_TEST)

#include "core/App.h"
#include "debug/Debug.h"

#include <Arduino.h>

namespace {
ewfm::App app;
}

void setup() {
    EWFM_DEBUG_BEGIN();
    app.begin();
}

void loop() {
    app.tick();
}

#endif
