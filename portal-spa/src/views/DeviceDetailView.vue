<template>
  <PageContainer>
    <template v-if="device">
      <PageCard>
        <template #header>
          <PageToolbar :title="deviceName" :subtitle="t(typeUi?.labelKey ?? 'device.type.unknown')" show-back @back="navigateBack">
            <template #actions>
              <v-tooltip :text="t(deviceStatusLabelKey(effectiveStatus))" location="top">
                <template #activator="{ props: tooltipProps }">
                  <v-avatar v-bind="tooltipProps" :size="10" :color="statusColor" />
                </template>
              </v-tooltip>
              <v-btn
                v-if="typeUi?.designerComponent"
                icon="$design-display"
                variant="text"
                size="small"
                :aria-label="t('device.dialog.openDesigner')"
                :to="{ name: 'device-design', params: { id: deviceId } }"
              />
            </template>
          </PageToolbar>
        </template>

        <div class="d-flex flex-column ga-4">
          <DeviceBaseFields :model-value="draft" mode="edit" :busy="loading" @update:model-value="draft = $event" />

          <v-divider v-if="typeUi" />

          <component
            :is="typeUi.fieldsComponent"
            v-if="typeUi && device"
            :model-value="draft"
            :device="device"
            mode="edit"
            :busy="loading"
            @update:model-value="draft = $event"
            @command="onCommand"
            @refresh="refresh"
          />

          <v-alert v-if="errorMessage" type="error" variant="tonal">
            {{ errorMessage }}
          </v-alert>
        </div>

        <template #actions>
          <v-btn variant="text" :disabled="loading" @click="navigateBack">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn color="primary" :loading="isSaving" :disabled="!canSave || loading" @click="onSave">
            {{ t('device.dialog.save') }}
          </v-btn>
        </template>
      </PageCard>

      <PageCard v-if="mqttStore.enabled && device.ha?.supported" class="mt-4">
        <template #header>
          <PageToolbar :title="t('device.ha.title')" :subtitle="haUniqueId ? t('device.ha.uniqueIdLabel', { id: haUniqueId }) : undefined" />
        </template>

        <div class="d-flex flex-column ga-4">
          <v-switch v-model="haForm.enabled" :label="t('device.ha.enabled')" inset hide-details />
          <v-text-field
            v-model="haForm.name"
            :label="t('device.ha.name')"
            :hint="t('device.ha.nameHint')"
            persistent-hint
            :placeholder="device?.config.name"
          />
        </div>

        <template #actions>
          <v-btn color="primary" size="small" :loading="haSaving" @click="onSaveHaSettings">
            {{ t('actions.save') }}
          </v-btn>
        </template>
      </PageCard>
    </template>

    <PageCard v-else-if="!loading">
      <div class="text-medium-emphasis">{{ t('notFound.title') }}</div>
    </PageCard>

    <div v-else class="d-flex justify-center pa-8">
      <v-progress-circular indeterminate color="primary" />
    </div>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onBeforeMount, reactive, ref, toRef, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'

import type { DeviceCommandRequest } from '@/api/contracts'
import { fetchMqttSettings, fetchMqttStatus } from '@/api'
import { deviceStatusColor, deviceStatusLabelKey } from '@/models/devices/device-status'
import { useDeviceDetail } from '@/composables/useDeviceDetail'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useMqttStore } from '@/stores/mqtt'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import { useNotificationsStore } from '@/stores/notifications'
import DeviceBaseFields from '@/components/device/DeviceBaseFields.vue'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const props = defineProps<{
  deviceId: number
}>()

const { t } = useI18n()
const router = useRouter()
const deviceStore = useDeviceRegistryStore()
const notifications = useNotificationsStore()
const mqttStore = useMqttStore()

const { device, deviceName, loading, isSaving, errorMessage, draft, canSave, refresh, save, submitCommand } =
  useDeviceDetail(toRef(props, 'deviceId'))

const haForm = reactive({ enabled: false, name: '' })
const haSaving = ref(false)

async function onSave(): Promise<void> {
  await save()
  if (!errorMessage.value) {
    notifications.notify(t('notifications.deviceSaved'), 'success')
  }
}

async function onCommand(payload: DeviceCommandRequest): Promise<void> {
  await submitCommand(payload)
}

async function onSaveHaSettings(): Promise<void> {
  haSaving.value = true
  try {
    await submitCommand({
      command: 'setHaSettings',
      haEnabled: haForm.enabled,
      haName: haForm.name,
    })
    if (!errorMessage.value) {
      notifications.notify(t('device.ha.saveSuccess'), 'success')
    }
  } finally {
    haSaving.value = false
  }
}

watch(device, value => {
  haForm.enabled = value?.ha?.enabled ?? false
  haForm.name = value?.ha?.name ?? ''
})

onBeforeMount(async () => {
  await deviceStore.initialize()
  await refresh()
  const mqttStatus = await fetchMqttStatus()
  mqttStore.replaceFromStatus(mqttStatus)
  if (mqttStatus.enabled) {
    // haNodeId lives in settings, not status - needed to show the real HA unique_id below.
    const mqttSettings = await fetchMqttSettings()
    mqttStore.replaceFromSettings(mqttSettings)
  }
})

const typeUi = computed(() => (device.value ? resolveDeviceUi(device.value.record.typeName) : null))

const haUniqueId = computed(() => {
  if (!device.value || !mqttStore.haNodeId) {
    return ''
  }
  return `${mqttStore.haNodeId}_${device.value.record.typeName}_${device.value.record.id}`
})

const effectiveStatus = computed(() => device.value?.runtime.effectiveStatus ?? device.value?.runtime.status ?? 'unknown')

const statusColor = computed(() => deviceStatusColor(effectiveStatus.value))

function navigateBack(): void {
  router.back()
}
</script>
