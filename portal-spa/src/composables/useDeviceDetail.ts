import { computed, ref, watch, type ComputedRef, type Ref } from 'vue'
import { commandDevice, fetchDevice, type DeviceCommandRequest, type DeviceRecord } from '@/api'
import {
  buildDeviceEditCommands,
  createDeviceEditDraft,
  type DeviceEditDraft,
} from '@/components/device/device-form'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { isValidDeviceName } from '@/models/devices/device-name'

// Home Assistant opt-in/name, edited independently of the main device form: it is persisted by the
// "setHaSettings" command into its own store, so it has no configRevision of its own.
export interface DeviceHaDraft {
  enabled: boolean
  name: string
}

export interface UseDeviceDetailReturn {
  device: ComputedRef<DeviceRecord | null>
  deviceName: ComputedRef<string>
  loading: ComputedRef<boolean>
  busyAction: Ref<'refresh' | 'save' | 'command' | null>
  isSaving: ComputedRef<boolean>
  errorMessage: Ref<string>
  draft: Ref<DeviceEditDraft>
  haDraft: Ref<DeviceHaDraft>
  canSave: ComputedRef<boolean>
  refresh(): Promise<void>
  save(payload?: DeviceEditDraft): Promise<void>
  submitCommand(payload: DeviceCommandRequest): Promise<void>
  resetDraft(): void
}

export function useDeviceDetail(deviceId: Ref<number>): UseDeviceDetailReturn {
  const deviceStore = useDeviceRegistryStore()

  const draft = ref<DeviceEditDraft>(createDeviceEditDraft(null))
  const haDraft = ref<DeviceHaDraft>({ enabled: false, name: '' })
  const busyAction = ref<'refresh' | 'save' | 'command' | null>(null)
  const errorMessage = ref('')

  const device = computed<DeviceRecord | null>(() => {
    const id = deviceId.value
    if (id <= 0) return null
    return deviceStore.devices.find(d => d.record.id === id) ?? null
  })

  const deviceName = computed(() => (device.value as any)?.config?.name ?? '')

  const loading = computed(() => busyAction.value !== null)

  const isSaving = computed(() => busyAction.value === 'save')

  const canSave = computed(() => {
    if (device.value === null) return false
    if (!isValidDeviceName(draft.value.name)) return false
    return buildDeviceEditCommands(device.value, draft.value).length > 0
  })

  // Reset the draft only when the device identity or its persisted config changes -- NOT on every
  // device.value reference change. Live runtime pushes (device.upsert / command_result) replace the
  // store object on each tick; keying the watch on the whole object would wipe unsaved edits (e.g.
  // an applied-but-unsaved calibration) on the next runtime update.
  watch(
    () => {
      const record = device.value?.record
      return record ? `${record.id}:${record.configRevision}` : null
    },
    key => {
      if (key !== null) {
        resetDraft()
      }
    },
    { immediate: true },
  )

  function resetDraft(): void {
    const next = createDeviceEditDraft(device.value)
    draft.value = next
  }

  // HA settings are persisted outside the device config and therefore have no configRevision.
  // Match the existing MQTT/Time settings-form pattern: copy the persisted value into the draft
  // only after an explicit REST refresh or a successful save, never after a runtime-only WS upsert.
  function applyHaSettingsToDraft(): void {
    haDraft.value = {
      enabled: device.value?.ha?.enabled ?? false,
      name: device.value?.ha?.name ?? '',
    }
  }

  async function refresh(): Promise<void> {
    if (deviceId.value <= 0) return
    busyAction.value = 'refresh'
    errorMessage.value = ''
    try {
      const response = await fetchDevice(deviceId.value)
      deviceStore.upsertDevice(response.device, response.registryRevision)
      applyHaSettingsToDraft()
    } catch (error) {
      errorMessage.value = formatError(error)
    } finally {
      busyAction.value = null
    }
  }

  async function save(payload?: DeviceEditDraft): Promise<void> {
    if (deviceId.value <= 0 || device.value === null) return
    busyAction.value = 'save'
    errorMessage.value = ''
    try {
      const payloadToUse = payload ?? draft.value
      const commands = buildDeviceEditCommands(device.value, payloadToUse)
      for (const command of commands) {
        const commandDeviceId = command.deviceId ?? deviceId.value
        const response = await commandDevice(commandDeviceId, {
          ...command,
          deviceId: commandDeviceId,
        })
        deviceStore.setRevision(response.registryRevision)
        if (response.device !== undefined) {
          deviceStore.upsertDevice(response.device, response.registryRevision)
        }
      }
      resetDraft()
    } catch (error) {
      errorMessage.value = formatError(error)
    } finally {
      busyAction.value = null
    }
  }

  async function submitCommand(payload: DeviceCommandRequest): Promise<void> {
    if (deviceId.value <= 0) return
    busyAction.value = 'command'
    errorMessage.value = ''
    try {
      const commandDeviceId = payload.deviceId ?? deviceId.value
      const response = await commandDevice(commandDeviceId, {
        ...payload,
        deviceId: commandDeviceId,
      })
      deviceStore.setRevision(response.registryRevision)
      if (response.device !== undefined) {
        deviceStore.upsertDevice(response.device, response.registryRevision)
      }
      if (payload.command === 'setHaSettings') {
        applyHaSettingsToDraft()
      }
    } catch (error) {
      errorMessage.value = formatError(error)
    } finally {
      busyAction.value = null
    }
  }

  return {
    device,
    deviceName,
    loading,
    busyAction,
    isSaving,
    errorMessage,
    draft,
    haDraft,
    canSave,
    refresh,
    save,
    submitCommand,
    resetDraft,
  }
}

function formatError(error: unknown): string {
  if (error instanceof Error) {
    return error.message
  }
  return 'Unknown error'
}
