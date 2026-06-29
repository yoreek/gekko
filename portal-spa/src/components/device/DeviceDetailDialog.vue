<template>
    <DeviceDialogShell
      :model-value="modelValue"
    :headline="deviceName ?? t('device.dialog.noneSelected')"
    :subline="device ? `${typeLabelText} · #${deviceId}` : undefined"
    :fullscreen="fullscreen"
    :max-width="980"
    @update:model-value="$emit('update:modelValue', $event)"
  >
    <template #title-actions>
      <v-chip v-if="device" variant="tonal" :color="statusColor">
        {{ statusText }}
      </v-chip>
      <v-chip v-if="device" variant="outlined" :color="editing ? 'primary' : 'secondary'">
        {{ editing ? t('device.dialog.edit') : t('labels.view') }}
      </v-chip>
      <v-btn
        v-if="device && !editing"
        class="device-dialog__icon-button"
        variant="text"
        :loading="busyAction === 'refresh'"
        :disabled="busy"
        :aria-label="t('actions.refresh')"
        @click="$emit('refresh')"
      >
        <v-icon icon="refresh" />
      </v-btn>
      <v-btn
        v-if="device && !editing"
        class="device-dialog__icon-button"
        variant="text"
        :disabled="busy"
        :aria-label="t('device.dialog.edit')"
        @click="enterEditMode"
      >
        <v-icon icon="edit" />
      </v-btn>
      <v-btn
        v-if="device && (device.record.typeName === 'ssd1306' || device.record.typeName === 'st7735') && !editing"
        class="device-dialog__icon-button"
        variant="text"
        :disabled="busy"
        :aria-label="device.record.typeName === 'st7735' ? t('device.dialog.st7735Display.designDisplay') : t('device.dialog.ssd1306Display.designDisplay')"
        @click="designerOpen = true"
      >
        <v-icon icon="design-display" />
      </v-btn>
    </template>

    <template v-if="device">
      <div class="device-dialog__content">
        <section class="device-dialog__section">
          <DeviceCommonFields
            v-model="draft"
            :mode="editing ? 'edit' : 'view'"
            :busy="busy"
          />
        </section>

        <component
          v-if="editing && editFormComponent"
          :is="editFormComponent"
          v-model="draft"
          mode="edit"
          :output-state="outputState"
          show-output-state
          :busy="busy"
          @design-display="designerOpen = true"
        />

        <v-alert
          v-if="layoutReviewWarning"
          type="warning"
          variant="tonal"
        >
          {{ layoutReviewWarning }}
        </v-alert>

        <component
          v-if="!editing && hasTypeDetails"
          :is="detailComponent"
          :device="device"
          :busy="busyAction === 'command'"
          @design-display="designerOpen = true"
          @command="$emit('command', $event)"
        />

        <section v-if="!editing && device" class="device-dialog__section">
          <RecentDeviceEvents :device-id="deviceId" />
        </section>
      </div>

    </template>

    <div v-else class="device-dialog__empty text-body-1">
      <span>{{ t('device.dialog.noneSelected') }}</span>
    </div>

    <template #footer>
      <v-alert v-if="errorMessage" class="device-dialog__error" type="error" variant="tonal">
        {{ errorMessage }}
      </v-alert>
      <v-spacer />
      <v-btn v-if="editing" variant="text" :disabled="busy" @click="cancelEdit">
        {{ t('actions.cancel') }}
      </v-btn>
      <v-btn v-if="editing" color="primary" :loading="busyAction === 'save'" :disabled="!canSave || busy" @click="submitSave">
        {{ t('device.dialog.save') }}
      </v-btn>
      <v-btn v-else variant="text" @click="$emit('update:modelValue', false)">
        {{ t('device.actions.close') }}
      </v-btn>
    </template>
  </DeviceDialogShell>

  <Ssd1306DesignerDialog
    v-if="device && device.record.typeName === 'ssd1306'"
    v-model="designerOpen"
    :device="device"
    @save="handleDesignerSave"
  />

  <St7735DesignerDialog
    v-else-if="device && device.record.typeName === 'st7735'"
    v-model="designerOpen"
    :device="device"
    @save="handleDesignerSave"
  />
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useDisplay } from 'vuetify'
import { useI18n } from 'vue-i18n'

import DeviceCommonFields from '@/components/device/DeviceCommonFields.vue'
import DeviceDialogShell from '@/components/device/DeviceDialogShell.vue'
import RecentDeviceEvents from '@/components/device/RecentDeviceEvents.vue'
import Ssd1306DesignerDialog from '@/components/devices/display/ssd1306/Ssd1306DesignerDialog.vue'
import St7735DesignerDialog from '@/components/devices/display/st7735/St7735DesignerDialog.vue'
import {
  buildDeviceEditCommands,
  createDeviceEditDraft,
  type DeviceEditDraft,
} from '@/components/device/device-form'
import { resolveDeviceDetailComponent, resolveDeviceFormComponent } from '@/components/devices/registry/device-component-registry'
import type { DeviceRecord } from '@/api/contracts'
import { deviceStatusLabelKey, deviceTypeLabelKeyByName } from '@/models/device-types'
import type { DeviceCommandRequest } from '@/api'

const props = defineProps<{
  modelValue: boolean
  device: DeviceRecord | null
  editing: boolean
  busyAction: 'refresh' | 'save' | 'command' | null
  errorMessage: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  'update:editing': [value: boolean]
  refresh: []
  save: [payload: DeviceEditDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const { smAndDown } = useDisplay()
const draft = ref<any>(createDeviceEditDraft(props.device ?? null))
const busy = computed(() => props.busyAction !== null)
const fullscreen = computed(() => smAndDown.value)
const designerOpen = ref(false)
const device = computed(() => props.device)
const hasTypeDetails = computed(() => device.value !== null)
const deviceId = computed(() => (device.value === null ? 0 : device.value.record.id))
const deviceName = computed(() => (device.value === null ? undefined : device.value.config.name))

const editFormComponent = computed(() => {
  if (device.value === null) {
    return null
  }
  return resolveDeviceFormComponent(device.value.record.typeName)
})

const detailComponent = computed(() => {
  if (device.value === null) {
    return null
  }
  return resolveDeviceDetailComponent(device.value.record.typeName)
})

const typeLabelText = computed(() => {
  if (device.value === null) {
    return ''
  }
  return t(deviceTypeLabelKeyByName(device.value.record.typeName))
})

const statusText = computed(() => {
  if (device.value === null) {
    return ''
  }
  const status = device.value.runtime.effectiveStatus ?? device.value.runtime.lifecycleStatus ?? device.value.runtime.status ?? 'unknown'
  return t(deviceStatusLabelKey(status))
})

const statusColor = computed(() => {
  if (device.value === null) {
    return 'primary'
  }
  switch (device.value.runtime.effectiveStatus ?? device.value.runtime.lifecycleStatus ?? device.value.runtime.status ?? 'unknown') {
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

const outputState = computed(() => {
  if (device.value === null || device.value.record.typeName !== 'gpio_switch') {
    return undefined
  }
  return (device.value.runtime as { state?: string }).state
})
const layoutReviewWarning = computed(() => {
  if (device.value === null || !props.editing || device.value.record.typeName !== 'ssd1306') {
    return ''
  }
  const currentConfig = device.value.config as unknown as Record<string, unknown>
  const currentWidth = Number(currentConfig.width ?? 0)
  const currentHeight = Number(currentConfig.height ?? 0)
  const draftWidth = Number(draft.value.width ?? 0)
  const draftHeight = Number(draft.value.height ?? 0)
  if (currentWidth === draftWidth && currentHeight === draftHeight) {
    return ''
  }
  return t('device.dialog.ssd1306Display.layoutResizeWarning')
})
const canSave = computed(() => {
  if (device.value === null) {
    return false
  }
  if (draft.value.name.trim().length === 0) {
    return false
  }
  return buildDeviceEditCommands(device.value, draft.value).length > 0
})

watch(
  () => props.device,
  current => {
    if (current !== null && !props.editing) {
      resetDrafts(current)
    }
  },
  { immediate: true },
)

watch(
  () => props.editing,
  editing => {
    if (editing && props.device !== null) {
      resetDrafts(props.device)
    }
    if (!editing && props.device !== null) {
      resetDrafts(props.device)
    }
  },
)

watch(
  () => props.modelValue,
  open => {
    if (!open) {
      designerOpen.value = false
    }
  },
)

function resetDrafts(current: DeviceRecord): void {
  const next = createDeviceEditDraft(current)
  draft.value = next
}

function enterEditMode(): void {
  if (device.value === null) {
    return
  }
  resetDrafts(device.value)
  emit('update:editing', true)
}

function cancelEdit(): void {
  if (device.value !== null) {
    resetDrafts(device.value)
  }
  emit('update:editing', false)
}

function submitSave(): void {
  if (!canSave.value || device.value === null) {
    return
  }
  const commands = buildDeviceEditCommands(device.value, {
    ...draft.value,
    name: draft.value.name.trim(),
  })
  if (commands.length === 0) {
    emit('update:editing', false)
    return
  }
  emit('save', {
    ...draft.value,
    name: draft.value.name.trim(),
  })
}

function handleDesignerSave(payload: Record<string, unknown>): void {
  emit('save', payload as DeviceEditDraft)
}

</script>

<style scoped>
.device-dialog__content {
  display: grid;
  gap: 12px;
}

.device-dialog__section {
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 10px;
  background: var(--portal-surface);
  box-shadow: var(--portal-shadow-sm);
}

.device-dialog__section + .device-dialog__section {
  margin-top: 4px;
}

.device-dialog__empty {
  padding: 40px 16px;
  text-align: center;
}

.device-dialog__error {
  flex: 1 1 auto;
}
</style>
