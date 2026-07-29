import {
  computed,
  ref,
  watch,
  type ComputedRef,
  type Ref,
  type WritableComputedRef,
} from 'vue'

import {
  commandDevice,
  fetchDevice,
  type DeviceCommandRequest,
  type DeviceRecord,
} from '@/api'
import {
  buildDeviceEditCommands,
  createDeviceEditDraft,
  type DeviceEditDraft,
} from '@/components/device/device-form'
import { useAsyncForm } from '@/composables/useAsyncForm'
import { isValidDeviceName } from '@/models/devices/device-name'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

export interface DeviceHaDraft {
  enabled: boolean
  name: string
}

interface DeviceDetailSource {
  device: DeviceRecord
  registryRevision: number
}

interface DeviceDetailDraft {
  config: DeviceEditDraft
  ha: DeviceHaDraft
}

export interface UseDeviceDetailReturn {
  device: ComputedRef<DeviceRecord | null>
  deviceName: ComputedRef<string>
  loading: ComputedRef<boolean>
  isSaving: ComputedRef<boolean>
  errorMessage: ComputedRef<string>
  draft: WritableComputedRef<DeviceEditDraft | null>
  haDraft: WritableComputedRef<DeviceHaDraft | null>
  canSave: ComputedRef<boolean>
  refresh(): Promise<void>
  save(payload?: DeviceEditDraft): Promise<void>
  submitCommand(payload: DeviceCommandRequest): Promise<void>
  resetDraft(): void
}

export function useDeviceDetail(deviceId: Ref<number>): UseDeviceDetailReturn {
  const deviceStore = useDeviceRegistryStore()
  const commandBusy = ref(false)
  const commandError = ref<unknown | null>(null)

  const lifecycle = useAsyncForm<DeviceDetailSource, DeviceDetailDraft>({
    load: async () => {
      const response = await fetchDevice(deviceId.value)
      return {
        device: response.device,
        registryRevision: response.registryRevision,
      }
    },
    createDraft: source => ({
      config: createDeviceEditDraft(source.device),
      ha: {
        enabled: source.device.ha?.enabled ?? false,
        name: source.device.ha?.name ?? '',
      },
    }),
    isDirty: (source, form) => (
      buildDeviceEditCommands(source.device, form.config).length > 0
      || isHaDirty(source.device, form.ha)
    ),
    validate: form => isValidDeviceName(form.config.name),
    save: saveDevice,
    onCommit: source => {
      deviceStore.upsertDevice(source.device, source.registryRevision)
    },
  })

  const device = computed<DeviceRecord | null>(() => {
    if (!lifecycle.ready.value) {
      return null
    }
    return deviceStore.devices.find(entry => entry.record.id === deviceId.value) ?? null
  })

  const deviceName = computed(() => device.value?.config.name ?? '')
  const loading = computed(() => (
    lifecycle.phase.value === 'idle'
    || lifecycle.busy.value
    || commandBusy.value
  ))
  const isSaving = computed(() => lifecycle.saving.value)
  const errorMessage = computed(() => formatError(
    commandError.value
    ?? lifecycle.saveError.value
    ?? lifecycle.loadError.value,
  ))

  const draft = computed({
    get: () => lifecycle.draft.value?.config ?? null,
    set: value => {
      if (value !== null && lifecycle.draft.value !== null) {
        lifecycle.draft.value.config = value
      }
    },
  })

  const haDraft = computed({
    get: () => lifecycle.draft.value?.ha ?? null,
    set: value => {
      if (value !== null && lifecycle.draft.value !== null) {
        lifecycle.draft.value.ha = value
      }
    },
  })

  watch(deviceId, () => {
    lifecycle.reset()
    void lifecycle.initialize()
  })

  async function saveDevice(
    { source, draft: form }: { source: DeviceDetailSource; draft: DeviceDetailDraft },
  ): Promise<DeviceDetailSource> {
    let nextDevice = source.device
    let registryRevision = source.registryRevision
    const commands = buildDeviceEditCommands(source.device, form.config)

    if (source.device.ha?.supported && isHaDirty(source.device, form.ha)) {
      commands.push({
        command: 'setHaSettings',
        haEnabled: form.ha.enabled,
        haName: form.ha.name,
      })
    }

    for (const command of commands) {
      const commandDeviceId = command.deviceId ?? deviceId.value
      const response = await commandDevice(commandDeviceId, {
        ...command,
        deviceId: commandDeviceId,
      })
      registryRevision = response.registryRevision
      deviceStore.setRevision(response.registryRevision)
      if (response.device !== undefined) {
        deviceStore.upsertDevice(response.device, response.registryRevision)
        if (commandDeviceId === deviceId.value) {
          nextDevice = deviceStore.devices.find(entry => entry.record.id === deviceId.value)
            ?? response.device
        }
      }
    }

    return { device: nextDevice, registryRevision }
  }

  async function refresh(): Promise<void> {
    commandError.value = null
    await lifecycle.refresh()
  }

  async function save(payload?: DeviceEditDraft): Promise<void> {
    commandError.value = null
    if (!lifecycle.ready.value && !await lifecycle.initialize()) {
      return
    }
    if (payload !== undefined && lifecycle.draft.value !== null) {
      lifecycle.draft.value.config = payload
    }
    await lifecycle.save()
  }

  async function submitCommand(payload: DeviceCommandRequest): Promise<void> {
    if (deviceId.value <= 0 || commandBusy.value || lifecycle.busy.value) {
      return
    }

    commandBusy.value = true
    commandError.value = null
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
      if (!lifecycle.dirty.value) {
        await lifecycle.refresh({ discardChanges: true })
      }
    } catch (error) {
      commandError.value = error
    } finally {
      commandBusy.value = false
    }
  }

  return {
    device,
    deviceName,
    loading,
    isSaving,
    errorMessage,
    draft,
    haDraft,
    canSave: lifecycle.canSave,
    refresh,
    save,
    submitCommand,
    resetDraft: lifecycle.resetDraft,
  }
}

function isHaDirty(device: DeviceRecord, draft: DeviceHaDraft): boolean {
  return (device.ha?.enabled ?? false) !== draft.enabled
    || (device.ha?.name ?? '') !== draft.name
}

function formatError(error: unknown): string {
  return error instanceof Error ? error.message : error ? String(error) : ''
}
