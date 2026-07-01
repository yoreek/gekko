<template>
  <v-container class="page-shell" fluid>
    <v-card class="page-card page-hero mb-4">
      <v-card-title class="page-title">
        <div>
          <div class="text-overline">{{ t('devices.title') }}</div>
          <h1 class="text-h5 sm:text-h4 font-weight-bold text-wrap">{{ t('device.dashboard.create') }}</h1>
        </div>
        <div class="d-flex ga-2">
          <v-btn
            icon="arrow-left"
            variant="text"
            :aria-label="t('device.actions.back')"
            @click="navigateBack"
          />
        </div>
      </v-card-title>
    </v-card>

    <v-card class="page-card">
      <v-card-text class="device-detail-content">
        <section class="device-detail-section">
          <DeviceCommonFields
            :model-value="(draft as any)"
            mode="create"
            :busy="isCreating ?? false"
            @update:model-value="(draft as any) = $event"
          />
        </section>

        <section class="device-detail-section">
          <component
            :is="(editorUi as any)?.editorComponent"
            v-if="(editorUi as any)?.editorComponent"
            :model-value="(draft as any)"
            mode="create"
            :busy="isCreating ?? false"
            @update:model-value="(draft as any) = $event"
          />
        </section>

        <!-- Error message -->
        <v-alert v-if="errorMessage" type="error" variant="tonal" class="mt-4">
          {{ errorMessage }}
        </v-alert>
      </v-card-text>
    </v-card>

    <!-- Sticky footer with actions -->
    <div class="device-detail-footer">
      <v-spacer />
      <v-btn variant="text" :disabled="(isCreating ?? false)" @click="navigateBack">
        {{ t('device.actions.cancel') }}
      </v-btn>
      <v-btn
        color="primary"
        :loading="isCreating ?? false"
        :disabled="(!canCreate || (isCreating ?? false)) ?? false"
        @click="submitCreate"
      >
        {{ t('device.dashboard.create') }}
      </v-btn>
    </div>
  </v-container>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'

import { createDevice, type DeviceCreateRequest } from '@/api'
import DeviceCommonFields from '@/components/device/DeviceCommonFields.vue'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import { createDefaultDeviceDraft } from '@/components/device/device-form'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const { t } = useI18n()
const router = useRouter()
const deviceStore = useDeviceRegistryStore()

const isCreating = ref(false)
const errorMessage = ref('')
const draft = ref(createDefaultDeviceDraft() as any)

const editorUi = computed(() => {
  if (!(draft.value as any).typeName) return null
  return resolveDeviceUi((draft.value as any).typeName)
})

const canCreate = computed(() => {
  const requiredFields = (draft.value as any).name && (draft.value as any).typeName
  return !!requiredFields
})

function navigateBack(): void {
  router.back()
}

async function submitCreate(): Promise<void> {
  if (!canCreate.value) return

  isCreating.value = true
  errorMessage.value = ''

  try {
    const payload = {
      name: draft.value.name,
      typeName: draft.value.typeName,
      config: draft.value,
    } as unknown as DeviceCreateRequest

    const response = await createDevice(payload)
    deviceStore.setRevision(response.registryRevision)
    if (response.device) {
      deviceStore.upsertDevice(response.device, response.registryRevision)
      await router.push({
        name: 'device-detail',
        params: { id: response.device.record.id },
      })
    }
  } catch (error) {
    errorMessage.value = error instanceof Error ? error.message : t('device.dialog.unknownError')
  } finally {
    isCreating.value = false
  }
}
</script>

<style scoped>
.device-detail-content {
  display: grid;
  gap: 12px;
}

.device-detail-section {
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 10px;
  background: var(--portal-surface);
  box-shadow: var(--portal-shadow-sm);
}

.device-detail-footer {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 16px 24px;
  background: var(--portal-surface);
  border-top: 1px solid var(--portal-border);
  box-shadow: 0 -2px 8px rgba(0, 0, 0, 0.1);
}
</style>
