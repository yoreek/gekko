import { BOARD_CATALOG, type BoardPinCapability, type ChipId, type PinRole } from '../../../data/board-pin-capabilities.ts'

// uint8_t sentinel "not configured" (see docs/pin-configuration-conventions.md) -- a legitimate
// transient value for mandatory-but-unset pins pre-save, not just for genuinely optional ones.
export const PIN_UNSET = 255

// Matches generate_boards.py's DEFAULT_CHIP -- the only chip this project actually compiles
// firmware for today (see platformio.ini). Once a second chip gets a real build target, callers
// that know which chip the connected device reports (SystemStatusResponse.chip.model) should pass
// it explicitly instead of relying on this default.
const DEFAULT_CHIP: ChipId = 'esp32'

// Reference board for *display* purposes only (labels, fixedDefaultFor, per-pin notes) -- there is
// no "which exact board is the user holding" selection anywhere yet, so this picks the classic-ESP32
// board with the widest pin breakout as a reasonable default. Validation never uses this -- it goes
// through chipPins()/isValidBoardPin() below, which mirror the firmware's chip-level (not per-board)
// table exactly.
const DEFAULT_BOARD_ID = 'nodemcu-32s'

export function boardPinDetails(boardId: string = DEFAULT_BOARD_ID): BoardPinCapability[] {
  return BOARD_CATALOG[boardId]?.pins ?? []
}

// Firmware validates pins at the *chip* level (silicon capability), not per physical board -- see
// BoardPinCapabilities.h's header comment. This mirrors that exact union-of-all-boards-for-a-chip
// computation in TS so normalizeConfig() never disagrees with what the firmware will accept, even
// though individual boards in BOARD_CATALOG only list the subset of pins their own header exposes.
function chipPins(chip: ChipId): { gpio: number; roles: PinRole[] }[] {
  const merged = new Map<number, Set<PinRole>>()
  for (const board of Object.values(BOARD_CATALOG)) {
    if (board.chip !== chip) continue
    for (const pin of board.pins) {
      const roles = merged.get(pin.gpio) ?? new Set<PinRole>()
      pin.roles.forEach(role => roles.add(role))
      merged.set(pin.gpio, roles)
    }
  }
  return [...merged.entries()]
    .sort(([a], [b]) => a - b)
    .map(([gpio, roles]) => ({ gpio, roles: [...roles] }))
}

export function boardPins(role?: PinRole, chip: ChipId = DEFAULT_CHIP) {
  const pins = chipPins(chip)
  return role ? pins.filter(pin => pin.roles.includes(role)) : pins
}

export function isValidBoardPin(value: number, role?: PinRole, chip: ChipId = DEFAULT_CHIP): boolean {
  return boardPins(role, chip).some(pin => pin.gpio === value)
}

// A device's defaultConfig() suggests a conventional pin (e.g. i2c_bus's sdaPin=21/sclPin=22 --
// ESP32's usual I2C pins) rather than forcing every new device through PIN_UNSET. But that literal
// default can already be claimed by another device (see docs/gpio-pin-occupancy.md) -- called once,
// right after a fresh create-draft is built, to fall back to PIN_UNSET rather than silently
// pre-filling an already-occupied pin. `owners` is usePinOccupancyStore().owners; never call this
// against an existing device's own persisted config (its own default is "occupied" by itself, which
// this cannot distinguish from a real conflict).
export function clearDefaultIfOccupied(pin: number, owners: Record<number, number>): number {
  return owners[pin] !== undefined ? PIN_UNSET : pin
}

// Shared normalizer for every pin field's normalizeConfig(): accepts PIN_UNSET as-is (mirrors the
// firmware's Category 2/3 sentinel handling) and any real chip GPIO with the required role;
// anything else -- wrong type, out of range, wrong role -- falls back. Mirrors
// gpioSwitchPinIsValid()/binarySensorPinIsValid()/analogPortInputGpioPinIsValid() in firmware via
// the same generated board-pin-capabilities data both sides consume.
export function normalizePin(value: unknown, fallback: number, role?: PinRole): number {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return fallback
  const rounded = Math.round(numeric)
  if (rounded === PIN_UNSET) return rounded
  return isValidBoardPin(rounded, role) ? rounded : fallback
}
