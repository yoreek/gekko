<template>
  <v-container class="page-shell" fluid>
    <!-- Not found state -->
    <div v-if="!loading && !device" class="not-found">
      <v-card class="not-found-card">
        <v-card-title>{{ t('notFound.title') }}</v-card-title>
        <v-card-text>{{ t('notFound.body') }}</v-card-text>
      </v-card>
    </div>

    <!-- Device detail page -->
    <template v-else-if="device">
      <div class="device-detail-page">
        <!-- Header -->
        <v-card class="page-card page-hero mb-4">
          <v-card-title class="page-title">
            <div>
              <div class="text-overline">{{ t('devices.title') }}</div>
              <h1 class="text-h5 sm:text-h4 font-weight-bold text-wrap">{{ (device as any)?.config?.name ?? '' }}</h1>
              <div class="text-body-2 text-medium-emphasis mt-1">
                {{ t(editorUi?.labelKey || 'device.type.unknown') }} · #{{ (device as any)?.record?.id ?? 0 }}
              </div>
            </div>
            <div class="d-flex ga-2 flex-wrap">
              <v-chip variant="tonal" :color="statusColor">
                {{ statusText }}
              </v-chip>
              <v-btn
                icon="arrow-left"
                variant="text"
                :aria-label="t('device.actions.close')"
                @click="navigateBack"
              />
              <v-btn
                v-if="editorUi?.designerComponent"
                icon="design-display"
                variant="text"
                :aria-label="t(editorUi?.designDisplayLabelKey || 'device.dialog.openDesigner')"
                @click="openDesigner"
              />
            </div>
          </v-card-title>
        </v-card>

        <!-- Device settings -->
        <v-card class="page-card">
          <v-card-text class="device-detail-content">
            <section class="device-detail-section">
              <DeviceCommonFields
                :model-value="(draft as any)"
                mode="edit"
                :busy="loading"
                @update:model-value="(v: any) => { draft.value = v }"
              />
            </section>

            <section class="device-detail-section">
              <component
                :is="(editorUi as any)?.editorComponent"
                v-if="(editorUi as any)?.editorComponent"
                :model-value="(draft as any)"
                :device="(device as any)"
                mode="edit"
                :busy="loading"
                @update:model-value="(v: any) => { draft.value = v }"
                @command="(submitCommand as any)"
              />
            </section>

            <!-- Recent events -->
            <RecentDeviceEvents v-if="device" :device-id="(device as any)?.record?.id ?? 0" />

            <!-- Error message -->
            <v-alert v-if="errorMessage" type="error" variant="tonal" class="mt-4">
              {{ errorMessage }}
            </v-alert>
          </v-card-text>
        </v-card>

        <!-- Sticky footer with actions -->
        <div class="device-detail-footer">
          <v-spacer />
          <v-btn variant="text" :disabled="loading" @click="(resetDraft as any)()">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn
            color="primary"
            :loading="busyAction === 'save'"
            :disabled="!canSave || loading"
            @click="(save as any)()"
          >
            {{ t('device.dialog.save') }}
          </v-btn>
        </div>

        <!-- Designer nested route overlay -->
        <router-view v-slot="{ Component, route }">
          <v-dialog
            v-if="route.name === 'device-design'"
            model-value
            fullscreen
            :transition="false"
          >
            <component :is="Component" />
          </v-dialog>
        </router-view>
      </div>
    </template>

    <!-- Loading state -->
    <div v-else class="d-flex align-center justify-center min-h-screen">
      <v-progress-circular indeterminate />
    </div>
  </v-container>
</template>

<script setup lang="ts">
import { computed, onBeforeMount, provide } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'

import DeviceCommonFields from '@/components/device/DeviceCommonFields.vue'
import RecentDeviceEvents from '@/components/device/RecentDeviceEvents.vue'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import { deviceStatusLabelKey } from '@/models/devices/device-status'
import { useDeviceDetail } from '@/composables/useDeviceDetail'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  deviceId: number
}>()

const { t } = useI18n()
const router = useRouter()
const deviceStore = useDeviceRegistryStore()

const deviceIdRef = computed(() => props.deviceId)

const detail = useDeviceDetail(deviceIdRef)
provide('deviceDetail', detail)

const { device, deviceName, loading, busyAction, isSaving, errorMessage, draft, canSave, refresh, save, submitCommand, resetDraft } = detail

onBeforeMount(async () => {
  console.log('DeviceDetailView mounted, deviceId:', props.deviceId)
  await deviceStore.initialize()
  console.log('Store initialized, devices count:', deviceStore.devices.length)
  await refresh()
  console.log('Detail refreshed, device:', device.value)
})

const editorUi = computed(() => {
  const dev = device.value as any
  if (!dev) return null
  const typeName = dev.record?.typeName
  if (!typeName) return null
  return resolveDeviceUi(typeName)
})

const statusText = computed(() => {
  const dev = device.value as any
  if (!dev?.runtime) return ''
  const status = (dev.runtime as any).effectiveStatus ?? (dev.runtime as any).lifecycleStatus ?? (dev.runtime as any).status ?? 'unknown'
  return t(deviceStatusLabelKey(status))
})

const statusColor = computed(() => {
  const dev = device.value as any
  if (!dev?.runtime) return 'primary'
  const status = (dev.runtime as any).effectiveStatus ?? (dev.runtime as any).lifecycleStatus ?? (dev.runtime as any).status ?? 'unknown'
  switch (status) {
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
  router.back()
}

function openDesigner(): void {
  const dev = device.value as any
  const id = dev?.record?.id
  if (id) router.push({ name: 'device-design', params: { id } })
}
</script>

<style scoped>
.not-found {
  padding: 48px 24px;
}

.not-found-card {
  padding: 24px;
}

.device-detail-page {
  position: relative;
  padding-bottom: 80px;
}

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

.min-h-screen {
  min-height: 100vh;
}
</style>
