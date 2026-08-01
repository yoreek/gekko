<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('device.dialog.createTitle')" show-back @back="navigateBack" />
      </template>

      <div class="d-flex flex-column ga-4">
        <DeviceBaseFields
          :model-value="draft"
          mode="create"
          :busy="isCreating"
          :name-error="duplicateNameError"
          @update:model-value="onBaseUpdate"
        />

        <v-divider v-if="typeUi" />

        <component
          :is="typeUi.fieldsComponent"
          v-if="typeUi"
          :model-value="draft"
          mode="create"
          :busy="isCreating"
          @update:model-value="onTypeUpdate"
        />

        <v-alert v-if="errorMessage" type="error" variant="tonal">
          {{ errorMessage }}
        </v-alert>
      </div>

      <template #actions>
        <v-btn variant="text" :disabled="isCreating" @click="navigateBack">
          {{ t('actions.cancel') }}
        </v-btn>
        <v-btn color="primary" :loading="isCreating" :disabled="!canCreate" @click="submitCreate">
          {{ t('actions.save') }}
        </v-btn>
      </template>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onBeforeMount, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRoute, useRouter } from 'vue-router'

import { createDevice } from '@/api'
import { createDefaultDeviceDraft, type DeviceCreateDraft } from '@/models/devices/device-draft'
import {
  DEFAULT_DEVICE_NAMES,
  isValidDeviceName,
  nextAvailableDeviceName,
} from '@/models/devices/device-name'
import { deviceTypeName } from '@/models/device-type-ids'
import { clearDefaultIfOccupied } from '@/models/devices/shared/pin'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore } from '@/stores/panels'
import { usePinOccupancyStore } from '@/stores/pinOccupancy'
import {
  allDeviceUis,
  resolveDeviceUi,
} from '@/components/devices/registry/device-ui-registry'
import { firstDeviceTypeOption } from '@/components/devices/registry/device-type-options'
import { resolveDeviceModelByTypeName } from '@/models/devices/device-model-factory'
import { useNotificationsStore } from '@/stores/notifications'
import DeviceBaseFields from '@/components/device/DeviceBaseFields.vue'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const route = useRoute()
const router = useRouter()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()
const notifications = useNotificationsStore()
const pinOccupancyStore = usePinOccupancyStore()

const targetPanelId = computed(() => {
  const value = route.query.panelId
  return typeof value === 'string' && value.length > 0 ? value : null
})

const isCreating = ref(false)
const errorMessage = ref('')
const defaultTypeId = firstDeviceTypeOption(allDeviceUis)?.typeId
const draft = ref<DeviceCreateDraft>(createDefaultDeviceDraft(deviceTypeName(defaultTypeId ?? 0)))

const typeUi = computed(() => (draft.value.typeName ? resolveDeviceUi(draft.value.typeName) : null))
const duplicateNameError = computed(() => isDuplicateDeviceName(draft.value.name) ? t('validation.uniqueDeviceName') : '')

const canCreate = computed(() =>
  isValidDeviceName(draft.value.name)
  && draft.value.typeName.length > 0
  && !isDuplicateDeviceName(draft.value.name))

// A device type's defaultConfig() may suggest a conventional pin (e.g. i2c_bus's sdaPin=21) that's
// already claimed by another device -- clearDefaultIfOccupied() falls back to PIN_UNSET rather than
// silently pre-filling an occupied pin. Only meaningful for types whose default is a real GPIO
// number, not PIN_UNSET already (see docs/gpio-pin-occupancy.md).
function clearOccupiedDefaultPins(target: DeviceCreateDraft): void {
  const owners = pinOccupancyStore.owners
  const config = target as Record<string, unknown>
  switch (target.typeName) {
    case 'i2c_bus':
      config.sdaPin = clearDefaultIfOccupied(config.sdaPin as number, owners)
      config.sclPin = clearDefaultIfOccupied(config.sclPin as number, owners)
      break
    case 'spi_bus':
      config.sckPin = clearDefaultIfOccupied(config.sckPin as number, owners)
      config.mosiPin = clearDefaultIfOccupied(config.mosiPin as number, owners)
      break
  }
}

onBeforeMount(async () => {
  await Promise.all([deviceStore.initialize(), pinOccupancyStore.refresh()])
  draft.value.name = createDefaultName(draft.value.typeName)
  clearOccupiedDefaultPins(draft.value)
})

function onBaseUpdate(value: DeviceCreateDraft): void {
  if (value.typeName !== draft.value.typeName) {
    draft.value = createDefaultDeviceDraft(value.typeName)
    draft.value.name = createDefaultName(value.typeName)
    draft.value.enabled = value.enabled
    clearOccupiedDefaultPins(draft.value)
    return
  }
  draft.value = value
}

function onTypeUpdate(value: DeviceCreateDraft): void {
  draft.value = value
}

function isDuplicateDeviceName(name: string): boolean {
  const normalizedName = name.trim().toLocaleLowerCase()
  return normalizedName.length > 0 && deviceStore.devices.some(device => device.config.name.trim().toLocaleLowerCase() === normalizedName)
}

function createDefaultName(typeName: DeviceCreateDraft['typeName']): string {
  return nextAvailableDeviceName(
    DEFAULT_DEVICE_NAMES[typeName],
    deviceStore.devices.map(device => device.config.name),
  )
}

function navigateBack(): void {
  router.back()
}

async function submitCreate(): Promise<void> {
  if (!canCreate.value) return

  isCreating.value = true
  errorMessage.value = ''

  try {
    const model = resolveDeviceModelByTypeName(draft.value.typeName)
    const payload = model.buildCreatePayload(draft.value)

    const response = await createDevice(payload)
    deviceStore.setRevision(response.registryRevision)
    if (response.device) {
      deviceStore.upsertDevice(response.device, response.registryRevision)
      notifications.notify(t('notifications.deviceCreated', { name: response.device.config.name }), 'success')
      // replace(), not push(): the create form shouldn't remain in history, so "back" from
      // the list doesn't return to a stale create form for an already-created device.
      if (targetPanelId.value) {
        panelStore.assignDeviceToPanel(targetPanelId.value, response.device.record.id)
        panelStore.setActivePanel(targetPanelId.value)
        await router.replace({ name: 'dashboard' })
      } else {
        await router.replace({ name: 'devices' })
      }
    }
  } catch (error) {
    errorMessage.value = error instanceof Error ? error.message : t('notifications.error')
  } finally {
    isCreating.value = false
  }
}
</script>
