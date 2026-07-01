<template>
  <v-container class="page-shell" fluid>
    <!-- Not found state -->
    <div v-if="!isLoading && !device" class="not-found">
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
              <h1 class="text-h5 sm:text-h4 font-weight-bold text-wrap">{{ deviceName }}</h1>
              <div class="text-body-2 text-medium-emphasis mt-1">
                {{ t(editorUi?.labelKey || 'device.type.unknown') }} · #{{ deviceId }}
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
                :model-value="(detail.draft as any)"
                mode="edit"
                :busy="unref(detail.loading) as boolean"
                @update:model-value="(v: any) => { (detail.draft as any) = v }"
              />
            </section>

            <section class="device-detail-section">
              <component
                :is="(editorUi as any)?.editorComponent"
                v-if="(editorUi as any)?.editorComponent"
                :model-value="(detail.draft as any)"
                :device="(device as any)"
                mode="edit"
                :busy="isLoading"
                @update:model-value="(v: any) => { (detail.draft as any) = v }"
                @command="(detail.submitCommand as any)"
              />
            </section>

            <!-- Recent events -->
            <RecentDeviceEvents v-if="device" :device-id="deviceId" />

            <!-- Error message -->
            <v-alert v-if="detail.errorMessage" type="error" variant="tonal" class="mt-4">
              {{ detail.errorMessage }}
            </v-alert>
          </v-card-text>
        </v-card>

        <!-- Sticky footer with actions -->
        <div class="device-detail-footer">
          <v-spacer />
          <v-btn variant="text" :disabled="unref(detail.loading) as boolean" @click="(detail.resetDraft as any)">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn
            color="primary"
            :loading="(unref(detail.busyAction) as any) === 'save'"
            :disabled="!(unref(detail.canSave) as boolean) || (unref(detail.loading) as boolean)"
            @click="(detail.save as any)()"
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
import { computed, onBeforeMount, provide, ref, unref } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'

import DeviceCommonFields from '@/components/device/DeviceCommonFields.vue'
import RecentDeviceEvents from '@/components/device/RecentDeviceEvents.vue'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import { deviceStatusLabelKey } from '@/models/devices/device-status'
import { useDeviceDetail } from '@/composables/useDeviceDetail'

const props = defineProps<{
  deviceId: number
}>()

const { t } = useI18n()
const router = useRouter()

const deviceIdRef = ref(props.deviceId)

const detail = useDeviceDetail(deviceIdRef)

onBeforeMount(async () => {
  await detail.refresh()
})

provide('deviceDetail', detail)

// Unwrap computed refs from composable for easier template access
const device = computed(() => detail.device as any)
const deviceName = computed(() => (device.value as any)?.config?.name ?? '')
const deviceId = computed(() => (device.value as any)?.record?.id ?? 0)
const deviceTypeName = computed(() => (device.value as any)?.record?.typeName ?? '')
const isLoading = computed(() => (detail.loading as any) ?? false)
const isSaving = computed(() => (detail.busyAction as any) === 'save')
const canSaveNow = computed(() => (detail.canSave as any) ?? false)

const editorUi = computed(() => {
  const typeName = (device.value as any)?.record?.typeName
  if (!typeName) return null
  return resolveDeviceUi(typeName)
})

const statusText = computed(() => {
  const dev = device.value
  if (!dev?.runtime) return ''
  const status = (dev.runtime as any).effectiveStatus ?? (dev.runtime as any).lifecycleStatus ?? (dev.runtime as any).status ?? 'unknown'
  return t(deviceStatusLabelKey(status))
})

const statusColor = computed(() => {
  const dev = device.value
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
  router.push({ name: 'device-design', params: { id: props.deviceId } })
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
