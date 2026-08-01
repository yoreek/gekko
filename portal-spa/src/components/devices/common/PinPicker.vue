<template>
  <v-select
    :label="label"
    :hint="hint"
    :persistent-hint="Boolean(hint)"
    :items="items"
    :model-value="modelValue"
    :readonly="readonly"
    :disabled="disabled"
    hide-details="auto"
    @update:model-value="update"
  />
</template>

<script setup lang="ts">
import { computed, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchBoardSettings } from '@/api'
import { boardPinDetails, PIN_UNSET } from '@/models/devices/shared/pin'
import type { PinRole } from '@/data/board-pin-capabilities'
import { useBoardStore } from '@/stores/board'
import { usePinOccupancyStore } from '@/stores/pinOccupancy'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  modelValue: number
  label: string
  hint?: string
  // Required capability for this field (e.g. 'output', 'adc1', 'input'). Pins lacking it are
  // shown disabled rather than hidden, so an already-saved-but-now-invalid value stays visible.
  requiredRole: PinRole
  // Allows the user to explicitly choose the firmware's "not configured" sentinel. Mandatory
  // fields still render their initial sentinel value, but do not offer it as a selectable option.
  allowUnset?: boolean
  // The device this field belongs to (undefined while creating a new device). Excludes that
  // device's own already-claimed pins from being reported as occupied by themselves.
  currentDeviceId?: number
  // GPIO numbers already chosen by OTHER pin fields on the same form/device draft (never this
  // field's own value). Catches same-device pin collisions (e.g. st7735 chipSelectPin == dcPin)
  // that usePinOccupancyStore can't see, since it only tracks cross-device ownership. PIN_UNSET
  // entries are ignored here -- several sibling fields may legitimately all be "unset" at once.
  siblingPins?: number[]
  readonly?: boolean
  disabled?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: number]
}>()

const { t } = useI18n()
const boardStore = useBoardStore()
const pinOccupancyStore = usePinOccupancyStore()
const deviceRegistryStore = useDeviceRegistryStore()

function roleBadge(roles: PinRole[]): string {
  return roles
    .filter(role => role !== 'output' && role !== 'input')
    .map(role => t(`device.dialog.pinRole.${role}`))
    .join(', ')
}

// Firmware-authoritative (see docs/gpio-pin-occupancy.md) -- undefined/self-owned pins aren't
// occupied from this field's point of view.
function occupantName(gpio: number): string | undefined {
  const ownerDeviceId = pinOccupancyStore.owners[gpio]
  if (ownerDeviceId === undefined || ownerDeviceId === props.currentDeviceId) return undefined
  return deviceRegistryStore.devices.find(device => device.record.id === ownerDeviceId)?.config.name
}

// PIN_UNSET is filtered here (rather than trusting every caller to do it) so several sibling
// fields can all sit at "not configured" simultaneously without tripping this check.
function claimedBySibling(gpio: number): boolean {
  return gpio !== PIN_UNSET && (props.siblingPins ?? []).includes(gpio)
}

const items = computed(() => {
  // Falls back to boardPinDetails()'s own hardcoded reference board until the board-settings
  // store has actually loaded (or on a real device whose board was never explicitly selected).
  const pins = boardPinDetails(boardStore.selectedBoardId || undefined).map(pin => {
    const compatible = pin.roles.includes(props.requiredRole)
    const badge = roleBadge(pin.roles)
    const fixedHint = pin.fixedDefaultFor ? ` (${t(`device.dialog.pinRole.fixedDefault`)})` : ''
    const occupant = compatible ? occupantName(pin.gpio) : undefined
    let itemProps: { disabled: boolean; subtitle?: string } | undefined
    if (!compatible) {
      itemProps = { disabled: true, subtitle: t('device.dialog.pinRole.incompatible') }
    } else if (occupant !== undefined) {
      itemProps = { disabled: true, subtitle: t('device.dialog.addressOccupiedBy', { name: occupant, id: pinOccupancyStore.owners[pin.gpio] }) }
    } else if (claimedBySibling(pin.gpio)) {
      itemProps = { disabled: true, subtitle: t('device.dialog.pinRole.usedByAnotherField') }
    }
    return {
      title: `GPIO${pin.gpio}${badge ? ` (${badge})` : ''}${fixedHint}`,
      value: pin.gpio,
      props: itemProps,
    }
  })
  // Keep an initial mandatory 255 value visible without making "not configured" selectable.
  // Optional fields opt in via allowUnset and get the same item enabled for user selection.
  if (props.allowUnset || props.modelValue === PIN_UNSET) {
    pins.unshift({
      title: t('device.dialog.pinRole.unset'),
      value: PIN_UNSET,
      props: props.allowUnset ? undefined : { disabled: true },
    })
  }
  return pins
})

function update(value: unknown): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return
  emit('update:modelValue', numeric)
}

// Board settings load lazily once per session (rarely changes). Pin occupancy is refetched on
// every mount instead -- it changes whenever any device anywhere is created/reconfigured/removed,
// so a session-long cache would go stale as soon as the user touches a second device form.
onMounted(() => {
  if (boardStore.selectedBoardId === '') {
    void fetchBoardSettings().then(settings => boardStore.replaceFromSettings(settings))
  }
  void pinOccupancyStore.refresh()
})
</script>
