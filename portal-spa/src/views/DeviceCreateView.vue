<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('device.dialog.createTitle')" :subtitle="t('device.dialog.createHint')" show-back @back="navigateBack" />
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
          {{ t('device.dialog.save') }}
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
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore } from '@/stores/panels'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
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

const targetPanelId = computed(() => {
  const value = route.query.panelId
  return typeof value === 'string' && value.length > 0 ? value : null
})

const isCreating = ref(false)
const errorMessage = ref('')
const draft = ref<DeviceCreateDraft>(createDefaultDeviceDraft())

const typeUi = computed(() => (draft.value.typeName ? resolveDeviceUi(draft.value.typeName) : null))
const duplicateNameError = computed(() => isDuplicateDeviceName(draft.value.name) ? t('validation.uniqueDeviceName') : '')

const canCreate = computed(() => draft.value.name.trim().length > 0 && draft.value.typeName.length > 0 && !isDuplicateDeviceName(draft.value.name))

onBeforeMount(async () => {
  await deviceStore.initialize()
  draft.value.name = nextAvailableDeviceName(draft.value.typeName)
})

function onBaseUpdate(value: DeviceCreateDraft): void {
  if (value.typeName !== draft.value.typeName) {
    draft.value = createDefaultDeviceDraft(value.typeName)
    draft.value.name = nextAvailableDeviceName(value.typeName)
    draft.value.enabled = value.enabled
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

function nextAvailableDeviceName(typeName: DeviceCreateDraft['typeName']): string {
  const baseName = t(resolveDeviceUi(typeName).labelKey)
  if (!isDuplicateDeviceName(baseName)) {
    return baseName
  }

  let suffix = 2
  while (isDuplicateDeviceName(`${baseName} ${suffix}`)) {
    suffix += 1
  }
  return `${baseName} ${suffix}`
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
    errorMessage.value = error instanceof Error ? error.message : t('device.dialog.unknownError')
  } finally {
    isCreating.value = false
  }
}
</script>
