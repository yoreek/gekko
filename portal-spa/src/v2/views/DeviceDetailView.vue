<template>
  <PageContainer>
    <template v-if="device">
      <PageToolbar :title="deviceName" :subtitle="t(typeUi?.labelKey ?? 'device.type.unknown')" show-back @back="navigateBack">
        <template #actions>
          <v-chip :color="statusColor" size="small" variant="tonal">
            {{ t(deviceStatusLabelKey(effectiveStatus)) }}
          </v-chip>
          <v-btn
            v-if="typeUi?.designerComponent"
            icon="$design-display"
            variant="text"
            size="small"
            :aria-label="t('device.dialog.openDesigner')"
            :to="{ name: 'v2-device-design', params: { id: deviceId } }"
          />
        </template>
      </PageToolbar>

      <v-card>
        <v-card-text class="d-flex flex-column ga-4">
          <DeviceBaseFields :model-value="draft" mode="edit" :busy="loading" @update:model-value="draft = $event" />

          <v-divider v-if="typeUi" />

          <component
            :is="typeUi.fieldsComponent"
            v-if="typeUi"
            :model-value="draft"
            mode="edit"
            :busy="loading"
            @update:model-value="draft = $event"
          />

          <v-alert v-if="errorMessage" type="error" variant="tonal">
            {{ errorMessage }}
          </v-alert>
        </v-card-text>

        <v-card-actions class="d-flex justify-end ga-3 flex-wrap">
          <v-btn variant="text" :disabled="loading" @click="resetDraft">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn color="primary" :loading="isSaving" :disabled="!canSave || loading" @click="save()">
            {{ t('device.dialog.save') }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </template>

    <v-card v-else-if="!loading">
      <v-card-text class="text-medium-emphasis">{{ t('notFound.title') }}</v-card-text>
    </v-card>

    <div v-else class="d-flex justify-center pa-8">
      <v-progress-circular indeterminate color="primary" />
    </div>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onBeforeMount } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'

import { deviceStatusLabelKey } from '@/models/devices/device-status'
import { useDeviceDetail } from '@/composables/useDeviceDetail'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { resolveDeviceUiV2 } from '@/v2/components/registry/device-ui-registry'
import DeviceBaseFields from '@/v2/components/device/DeviceBaseFields.vue'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'

const props = defineProps<{
  deviceId: number
}>()

const { t } = useI18n()
const router = useRouter()
const deviceStore = useDeviceRegistryStore()

const deviceIdRef = computed(() => props.deviceId)
const { device, deviceName, loading, isSaving, errorMessage, draft, canSave, refresh, save, resetDraft } =
  useDeviceDetail(deviceIdRef)

onBeforeMount(async () => {
  await deviceStore.initialize()
  await refresh()
})

const typeUi = computed(() => (device.value ? resolveDeviceUiV2(device.value.record.typeName) : null))

const effectiveStatus = computed(() => device.value?.runtime.effectiveStatus ?? device.value?.runtime.status ?? 'unknown')

const statusColor = computed(() => {
  switch (effectiveStatus.value) {
    case 'ready':
      return 'success'
    case 'disabled':
      return 'secondary'
    case 'faulted':
      return 'error'
    case 'dependency_blocked':
      return 'warning'
    default:
      return 'primary'
  }
})

function navigateBack(): void {
  router.push({ name: 'v2-devices' })
}
</script>
