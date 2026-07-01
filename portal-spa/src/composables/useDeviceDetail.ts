import { computed, ref, watch, type ComputedRef, type Ref } from 'vue'
import { commandDevice, fetchDevice, type DeviceCommandRequest, type DeviceRecord } from '@/api'
import {
  buildDeviceEditCommands,
  createDeviceEditDraft,
  type DeviceEditDraft,
} from '@/components/device/device-form'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

export interface UseDeviceDetailReturn {
  device: ComputedRef<DeviceRecord | null>
  loading: Ref<boolean>
  busyAction: Ref<'refresh' | 'save' | 'command' | null>
  errorMessage: Ref<string>
  draft: Ref<DeviceEditDraft>
  canSave: ComputedRef<boolean>
  refresh(): Promise<void>
  save(payload?: DeviceEditDraft): Promise<void>
  submitCommand(payload: DeviceCommandRequest): Promise<void>
  resetDraft(): void
}

export function useDeviceDetail(deviceId: Ref<number>): UseDeviceDetailReturn {
  const deviceStore = useDeviceRegistryStore()

  const draft = ref<DeviceEditDraft>(createDeviceEditDraft(null))
  const busyAction = ref<'refresh' | 'save' | 'command' | null>(null)
  const errorMessage = ref('')

  const device = computed<DeviceRecord | null>(() => {
    const id = deviceId.value
    if (id <= 0) return null
    return deviceStore.devices.find(d => d.record.id === id) ?? null
  })

  const loading = computed(() => busyAction.value !== null)

  const canSave = computed(() => {
    if (device.value === null) return false
    if (draft.value.name.trim().length === 0) return false
    return buildDeviceEditCommands(device.value, draft.value).length > 0
  })

  watch(
    () => device.value,
    current => {
      if (current !== null) {
        resetDraft()
      }
    },
    { immediate: true },
  )

  function resetDraft(): void {
    const next = createDeviceEditDraft(device.value)
    draft.value = next
  }

  async function refresh(): Promise<void> {
    if (deviceId.value <= 0) return
    busyAction.value = 'refresh'
    errorMessage.value = ''
    try {
      const response = await fetchDevice(deviceId.value)
      deviceStore.upsertDevice(response.device, response.registryRevision)
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
        const response = await commandDevice(deviceId.value, {
          ...command,
          deviceId: deviceId.value,
        })
        deviceStore.setRevision(response.registryRevision)
        if (response.device !== undefined) {
          deviceStore.upsertDevice(response.device, response.registryRevision)
        }
      }
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
      const response = await commandDevice(deviceId.value, {
        ...payload,
        deviceId: deviceId.value,
      })
      deviceStore.setRevision(response.registryRevision)
      if (response.device !== undefined) {
        deviceStore.upsertDevice(response.device, response.registryRevision)
      }
    } catch (error) {
      errorMessage.value = formatError(error)
    } finally {
      busyAction.value = null
    }
  }

  return {
    device,
    loading,
    busyAction,
    errorMessage,
    draft,
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
