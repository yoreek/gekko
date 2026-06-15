<template>
  <DeviceDialogShell
    :model-value="modelValue"
    :headline="device?.name ?? t('device.dialog.noneSelected')"
    :subline="device ? `${typeLabelText} · #${device.deviceId}` : undefined"
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
        <AppIcon name="refresh" />
      </v-btn>
      <v-btn
        v-if="device && !editing"
        class="device-dialog__icon-button"
        variant="text"
        :disabled="busy"
        :aria-label="t('device.dialog.edit')"
        @click="enterEditMode"
      >
        <AppIcon name="edit" />
      </v-btn>
    </template>

    <template v-if="device">
      <div class="device-dialog__content">
        <section class="device-dialog__section">
          <DeviceCommonFields
            v-model="draft.common"
            :mode="editing ? 'edit' : 'view'"
            :busy="busy"
          />
        </section>

        <GpioSwitchDeviceForm
          v-if="editing && isGpioSwitch"
          v-model="draft.gpioSwitchConfig"
          :output-state="outputState"
          show-output-state
          :busy="busy"
        />

        <component
          v-else-if="!editing && hasTypeDetails"
          :is="detailComponent"
          :device="device"
          :busy="busyAction === 'command'"
          @command="$emit('command', $event)"
        />
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
</template>

<script setup lang="ts">
import { computed, reactive, watch } from 'vue'
import { useDisplay } from 'vuetify'
import { useI18n } from 'vue-i18n'

import AppIcon from '@/components/AppIcon.vue'
import DeviceCommonFields from '@/components/device/DeviceCommonFields.vue'
import DeviceDialogShell from '@/components/device/DeviceDialogShell.vue'
import { buildDeviceEditCommands, createDeviceEditDraft, isGpioSwitchType } from '@/components/device/device-form'
import GpioSwitchDeviceForm from '@/components/devices/gpio-switch/GpioSwitchDeviceForm.vue'
import { resolveDeviceDetailComponent } from '@/components/devices/registry/device-component-registry'
import type { DashboardDevice } from '@/models/device'
import { deviceTypeLabelKey, DUMMY_DEVICE_TYPE_ID } from '@/models/device-types'
import type { DeviceCommandRequest } from '@/api'
import type { DeviceEditSubmitPayload } from '@/components/device/device-form'

const props = defineProps<{
  modelValue: boolean
  device: DashboardDevice | null
  editing: boolean
  busyAction: 'refresh' | 'save' | 'command' | null
  errorMessage: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  'update:editing': [value: boolean]
  refresh: []
  save: [payload: DeviceEditSubmitPayload]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const { smAndDown } = useDisplay()
const draft = reactive(createDeviceEditDraft(props.device ?? null))
const busy = computed(() => props.busyAction !== null)
const fullscreen = computed(() => smAndDown.value)
const device = computed(() => props.device)
const isGpioSwitch = computed(() => device.value !== null && isGpioSwitchType(device.value.typeId))
const hasTypeDetails = computed(() => device.value !== null && device.value.typeId !== DUMMY_DEVICE_TYPE_ID)

const detailComponent = computed(() => {
  if (device.value === null) {
    return null
  }
  return resolveDeviceDetailComponent(device.value.typeId)
})

const typeLabelText = computed(() => {
  if (device.value === null) {
    return ''
  }
  return t(deviceTypeLabelKey(device.value.typeId))
})

const statusText = computed(() => {
  if (device.value === null) {
    return ''
  }
  return device.value.backendEffectiveStatus || device.value.lifecycleStatus
})

const statusColor = computed(() => {
  if (device.value === null) {
    return 'primary'
  }
  switch (device.value.backendEffectiveStatus) {
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

const outputState = computed(() => device.value?.output.state)
const canSave = computed(() => {
  if (device.value === null) {
    return false
  }
  if (draft.common.name.trim().length === 0) {
    return false
  }
  const nextName = draft.common.name.trim()
  const currentDraft = createDeviceEditDraft(device.value)
  if (nextName !== currentDraft.common.name || draft.common.enabled !== currentDraft.common.enabled) {
    return true
  }
  if (isGpioSwitch.value) {
    return JSON.stringify(draft.gpioSwitchConfig) !== JSON.stringify(currentDraft.gpioSwitchConfig)
  }
  return false
})

watch(
  () => props.device,
  current => {
    if (current !== null) {
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

function resetDrafts(current: DashboardDevice): void {
  const next = createDeviceEditDraft(current)
  draft.common.name = next.common.name
  draft.common.typeId = next.common.typeId
  draft.common.enabled = next.common.enabled
  draft.gpioSwitchConfig = next.gpioSwitchConfig
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
    common: {
      name: draft.common.name.trim(),
      enabled: draft.common.enabled,
      typeId: draft.common.typeId,
    },
    gpioSwitchConfig: isGpioSwitch.value ? draft.gpioSwitchConfig : undefined,
  })
  if (commands.length === 0) {
    emit('update:editing', false)
    return
  }
  emit('save', {
    common: {
      name: draft.common.name.trim(),
      enabled: draft.common.enabled,
      typeId: draft.common.typeId,
    },
    gpioSwitchConfig: isGpioSwitch.value ? draft.gpioSwitchConfig : undefined,
  })
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

.device-dialog__empty {
  padding: 40px 16px;
  text-align: center;
}

.device-dialog__error {
  flex: 1 1 auto;
}
</style>
