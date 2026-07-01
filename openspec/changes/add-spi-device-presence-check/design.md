## Context

The SPI bus component (`SpiBusDevice`) is device-agnostic. CS pin ownership is distributed: each consumer (e.g., ST7735) manages its own CS pin directly via Arduino `digitalWrite`. When probing for device presence, we must respect CS-pin ownership in three scenarios:

1. **New device** — CS pin not yet initialized; free to use completely.
2. **Existing device** — CS pin already configured; must save state, modify temporarily, restore.
3. **Another device's pin** — CS pin already claimed by different device on same bus; must reject.

**Architecture baseline:**
- `DependencyTransaction` provides exclusive SPI-bus access (prevents concurrent SPI transfers).
- CS-pin is GPIO, managed by consumer independently; `DependencyTransaction` does **not** lock GPIO.
- `hasDuplicateDependentSpiChipSelect()` already validates pin uniqueness at config level; we must do runtime check too.

## Goals / Non-Goals

**Goals:**
- Safely probe CS pin in all three scenarios without disrupting active device operation.
- Save and restore CS state so existing device continues working post-probe.
- Detect and reject probes on pins claimed by other devices.
- Use MISO-activity (primary) and CS pull-resistor heuristic (secondary) as methods.
- Expose outcome (Detected/NotDetected/Inconclusive) and method in runtime JSON.

**Non-Goals:**
- Modify `SpiBusDeviceConfigV1` or device config structs (probe is transient).
- Guarantee device presence (heuristic only; user must understand limitations).
- Support SPI clock/mode configuration (outside scope of this feature).

## Decisions

### 1. State-preserving CS probe via DependencyTransaction

**Decision:** Probe acquires `DependencyTransaction`, saves CS state, temporarily modifies CS, restores state, releases transaction.

**Rationale:**
- `DependencyTransaction` ensures no other consumer is actively using SPI bus during probe.
- Save/restore CS ensures device (if already initialized) continues in same state post-probe.
- Minimal window of disruption (few microseconds for idle bytes + GPIO reads).

**Implementation sketch:**
```cpp
bool probeChipSelect(uint8_t csPin) {
    if (status_ != Ready || dependencyTransactionActive_) return false;
    
    // Collision check
    for (auto dependent : dependents_) {
        if (dependent->spiChipSelectPin(csPin)) return false;  // pin in use
    }
    
    auto txn = beginDependencyTransaction();
    if (!txn) return false;  // couldn't acquire bus
    
    // Save current state
    uint8_t savedMode = savedCsMode_;  // OUTPUT or INPUT
    bool savedValue = digitalRead(csPin);
    
    // Probe
    SpiProbeOutcome outcome = probeViaMisoActivity(csPin);
    if (outcome == Unknown && config_.misoPin < 0) {
        outcome = probeViaCsPullHeuristic(csPin);
    }
    
    // Restore
    if (savedMode == OUTPUT) {
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, savedValue);
    } else {
        pinMode(csPin, INPUT);  // or INPUT_PULLUP depending on original
    }
    
    txn.release();
    probe_ = {csPin, outcome, method, now, true};
    return true;
}
```

**Alternatives considered:**
- Don't save/restore: too risky; could crash device or corrupt data.
- Only allow probing before device init: too restrictive; defeats edit-scenario purpose.

### 2. Collision detection via `spiChipSelectPin()` callback

**Decision:** Query each dependent device for its CS pin; reject if match found.

**Rationale:** `hasDuplicateDependentSpiChipSelect()` pattern already exists; reuse it at probe time to ensure runtime safety.

**Alternatives considered:**
- Config-level validation only: insufficient; device could be reconfigured mid-probe.
- Ignore collisions: too dangerous; could corrupt peer device state.

### 3. MISO-activity primary, CS pull-heuristic secondary

**Decision:** Always try MISO-activity first (if `misoPin >= 0`). Fall back to CS pull-heuristic only if MISO unavailable or pin is free.

**MISO-activity (safe, high confidence):**
- Transfer idle bytes; examine MISO response
- Does not modify CS state (restore not needed for MISO-only check)
- Works in all scenarios (device init, configured, or free)

**CS pull-heuristic (medium risk, lower confidence):**
- Temporarily configure CS as INPUT_PULLUP, read level
- Then INPUT_PULLDOWN, read level
- If readings differ or show strong pull → heuristic evidence of device
- Only safe if CS state is known and can be restored (not during active render)
- Return `Inconclusive` more often than false confidence

**Rationale:**
- MISO is passive observation; cannot interfere with peer device.
- CS pull works only when we control the pin or can restore it.
- Heuristic-only; UI must caveat it clearly.

**Alternatives considered:**
- Clock out bits and measure current: requires ADC hardware.
- Assume presence from config: useless (feature's entire point is to verify).

### 4. Three-state outcome (Detected / NotDetected / Inconclusive)

**Decision:** Outcome + method pair allows clear UI labeling of confidence and technique.

**Rationale:**
- `Detected`: High confidence (MISO activity or strong CS pull signature).
- `NotDetected`: No signal detected; device likely absent (but could be unresponsive).
- `Inconclusive`: MISO shows some response but weak, or CS pull ambiguous. User should manually verify.
- `method`: Tells UI "this was MISO (reliable)" vs. "this was CS pull (heuristic)".

**Alternatives considered:**
- Boolean present/absent: Too coarse; doesn't convey confidence.
- Numeric score: False precision.

### 5. New ISpiCsProbeDriver interface

**Decision:** Dedicated interface for GPIO control with state management.

**Interface:**
- `readCurrentState(pin, &mode, &value)` — capture current pin direction and level
- `configureOutput(pin, level)` — set to OUTPUT, drive specific level
- `configureInputPullup(pin)` / `configureInputPulldown(pin)` — set mode, read level
- `restoreState(pin, mode, value)` — return to saved mode and level
- `release(pin)` — safety restore to INPUT if something went wrong

**Rationale:**
- Isolates GPIO logic from SpiBusDevice.
- Mockable for testing.
- Mirrors `ISpiBusDriver` pattern.

**Alternatives considered:**
- Inline digitalWrite calls: Couples to Arduino; harder to test.
- Use IGpioOutputDriver: Lacks input-mode support.

## Risks / Trade-offs

**Risk: Concurrent render during probe**
- *Scenario:* User probes ST7735 while display is actively rendering (even though `dependencyTransactionActive_ == false`).
- *Mitigation:* `DependencyTransaction` guard prevents SPI-peripheral interference. GPIO change (CS) might momentarily corrupt a single frame. Accept as known limitation; UI should warn "avoid during active use."

**Risk: Weak CS pull-heuristic**
- *Scenario:* Board has external 100kΩ pull-up; on-device pull is 10kΩ to GND; voltage divider reads as "no strong pull."
- *Mitigation:* Always return `Inconclusive` for CS pull. Label clearly in UI as heuristic. Recommend MISO config.

**Risk: Pin mode restoration edge case**
- *Scenario:* Device was in INPUT_PULLUP, we restore to INPUT (forgot pullup flag).
- *Mitigation:* `ISpiCsProbeDriver::readCurrentState()` must capture full state (including pullup enabled/disabled). `restoreState()` replicates exactly.

**Trade-off: Synchronous vs. queued probe**
- *Trade-off:* Synchronous inside `handleCommand()` is simple but blocks tick() briefly. Async would be smoother but adds state complexity.
- *Resolution:* Accept brief block; probes are <1ms and rare (user-triggered).

## Open Questions

- Should we log probe operations (success/failure/outcome) to a debug channel for diagnostics? (Nice-to-have; defer if time-constrained.)
- Should probe results trigger a device state refresh or config revalidation? (No; probe is read-only diagnostic.)
