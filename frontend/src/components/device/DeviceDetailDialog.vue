<template>
  <v-dialog
    :model-value="modelValue"
    max-width="980"
    scrollable
    :fullscreen="fullscreen"
    @update:model-value="$emit('update:modelValue', $event)"
  >
    <v-card class="device-dialog">
      <v-card-title class="device-dialog__title">
        <div>
          <div class="device-dialog__eyebrow">{{ t('device.dialog.title') }}</div>
          <div class="device-dialog__headline">{{ device?.name ?? t('device.dialog.noneSelected') }}</div>
          <div class="device-dialog__subline" v-if="device">
            {{ device.typeLabel }} · #{{ device.deviceId }}
          </div>
        </div>
        <div class="device-dialog__title-actions">
          <v-chip v-if="device" size="small" variant="tonal" :color="statusColor">
            {{ statusText }}
          </v-chip>
          <v-btn class="device-dialog__icon-button" variant="text" :loading="busyAction === 'refresh'" @click="$emit('refresh')">
            <AppIcon name="refresh" />
          </v-btn>
          <v-btn class="device-dialog__icon-button" variant="text" @click="$emit('update:modelValue', false)">
            <AppIcon name="close" />
          </v-btn>
        </div>
      </v-card-title>

      <v-divider />

      <v-card-text class="device-dialog__body">
        <template v-if="device">
          <section class="device-dialog__section">
            <div class="device-dialog__section-title">{{ t('device.dialog.commonFields') }}</div>
            <v-row dense>
              <v-col v-for="field in baseFieldRows" :key="field.key" cols="12" md="6">
                <div class="device-field">
                  <span>{{ field.label }}</span>
                  <strong>{{ field.value }}</strong>
                </div>
              </v-col>
            </v-row>
          </section>

          <section class="device-dialog__section">
            <div class="device-dialog__section-title">{{ t('device.dialog.actions') }}</div>
            <v-row dense>
              <v-col cols="12" md="8">
                <v-text-field
                  v-model="renameDraft"
                  :label="t('device.actions.renameLabel')"
                  :placeholder="device.name"
                  :disabled="busy"
                  density="comfortable"
                  hide-details
                />
              </v-col>
              <v-col cols="12" md="4" class="device-dialog__action-col">
                <v-btn
                  block
                  color="primary"
                  :loading="busyAction === 'rename'"
                  :disabled="busy || renameDraft.trim().length === 0 || renameDraft.trim() === device.name"
                  @click="submitRename"
                >
                  {{ t('device.actions.rename') }}
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  variant="tonal"
                  :loading="busyAction === 'toggle'"
                  :disabled="busy"
                  @click="toggleEnabled"
                >
                  {{ device.enabled ? t('device.actions.disable') : t('device.actions.enable') }}
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="error"
                  variant="outlined"
                  :loading="busyAction === 'delete'"
                  :disabled="busy"
                  @click="confirmDelete = true"
                >
                  {{ t('device.actions.delete') }}
                </v-btn>
              </v-col>
            </v-row>
          </section>

          <section class="device-dialog__section">
            <div class="device-dialog__section-title">{{ t('device.dialog.details') }}</div>
            <div v-if="device.kind === 'dummy'" class="typed-panel">
              <div class="typed-panel__header">
                <div>
                  <div class="typed-panel__title">{{ t('device.type.dummy') }}</div>
                  <div class="typed-panel__subtitle">{{ t('device.dialog.dummyHint') }}</div>
                </div>
                <v-chip size="small" variant="tonal">
                  {{ device.detail.outputState ? t('device.dialog.outputOn') : t('device.dialog.outputOff') }}
                </v-chip>
              </div>

              <div class="typed-panel__grid">
                <div class="device-field">
                  <span>{{ t('device.fields.retainedStateSupported') }}</span>
                  <strong>{{ yesNo(device.detail.retainedStateSupported) }}</strong>
                </div>
                <div class="device-field">
                  <span>{{ t('device.fields.restorePreviousState') }}</span>
                  <strong>{{ yesNo(device.detail.restorePreviousState) }}</strong>
                </div>
                <div class="device-field">
                  <span>{{ t('device.fields.currentOutput') }}</span>
                  <strong>{{ yesNo(device.detail.outputState) }}</strong>
                </div>
                <div class="device-field">
                  <span>{{ t('device.fields.defaultOutput') }}</span>
                  <strong>{{ yesNo(device.detail.retainedStartupFallbackOutput) }}</strong>
                </div>
              </div>

              <div class="typed-panel__commands">
                <div class="typed-panel__commands-title">{{ t('device.dialog.quickCommands') }}</div>
                <div class="typed-panel__command-row">
                  <v-btn
                    v-for="preset in commandPresets"
                    :key="preset.key"
                    :color="preset.tone === 'error' ? 'error' : preset.tone === 'primary' ? 'primary' : undefined"
                    :variant="preset.tone === 'primary' ? 'flat' : 'tonal'"
                    :loading="busyAction === 'command' && activePresetKey === preset.key"
                    :disabled="busy"
                    @click="runPreset(preset)"
                  >
                    {{ t(preset.labelKey) }}
                  </v-btn>
                </div>
              </div>
            </div>

            <div v-else class="generic-panel">
              <div class="device-dialog__section-copy">{{ t('device.dialog.genericHint') }}</div>
              <v-row dense>
                <v-col cols="12" md="4">
                  <v-select
                    v-model="commandDraft.command"
                    :items="commandOptions"
                    :label="t('device.actions.command')"
                    density="comfortable"
                    hide-details
                  />
                </v-col>
                <v-col cols="12" md="8">
                  <v-text-field
                    v-model="commandDraft.payload"
                    :label="t('device.actions.payload')"
                    :placeholder="t('device.actions.payloadPlaceholder')"
                    density="comfortable"
                    hide-details
                  />
                </v-col>
                <v-col v-if="commandDraft.command === 'set_parent'" cols="12" md="6">
                  <v-switch
                    v-model="commandDraft.hasParent"
                    :label="t('device.actions.hasParent')"
                    hide-details
                    inset
                  />
                </v-col>
                <v-col v-if="commandDraft.command === 'set_parent'" cols="12" md="6">
                  <v-text-field
                    v-model="commandDraft.parentDeviceId"
                    :label="t('device.actions.parentDeviceId')"
                    density="comfortable"
                    hide-details
                  />
                </v-col>
              </v-row>
              <div class="generic-panel__actions">
                <v-btn color="primary" :loading="busyAction === 'command'" :disabled="busy" @click="submitCommand">
                  {{ t('device.actions.sendCommand') }}
                </v-btn>
              </div>
            </div>
          </section>
        </template>

        <div v-else class="device-dialog__empty">
          <span>{{ t('device.dialog.noneSelected') }}</span>
        </div>
      </v-card-text>

      <v-divider />

      <v-card-actions class="device-dialog__footer">
        <v-alert v-if="errorMessage" class="device-dialog__error" type="error" variant="tonal" density="compact">
          {{ errorMessage }}
        </v-alert>
        <v-spacer />
        <v-btn variant="text" @click="$emit('update:modelValue', false)">
          {{ t('device.actions.close') }}
        </v-btn>
      </v-card-actions>
    </v-card>

    <v-dialog v-model="confirmDelete" max-width="420">
      <v-card>
        <v-card-title>{{ t('device.dialog.deleteConfirmTitle') }}</v-card-title>
        <v-card-text>{{ t('device.dialog.deleteConfirmBody') }}</v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="confirmDelete = false">{{ t('actions.cancel') }}</v-btn>
          <v-btn color="error" :loading="busyAction === 'delete'" @click="submitDelete">{{ t('device.actions.delete') }}</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed, reactive, ref, watch } from 'vue'
import { useDisplay } from 'vuetify'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest } from '@/api'
import AppIcon from '@/components/AppIcon.vue'
import { deviceActionPresets, type DashboardDevice, type DashboardDeviceActionPreset } from '@/models/device'

const props = defineProps<{
  modelValue: boolean
  device: DashboardDevice | null
  busyAction: 'refresh' | 'rename' | 'toggle' | 'delete' | 'command' | null
  errorMessage: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  refresh: []
  rename: [name: string]
  'toggle-enabled': [enabled: boolean]
  delete: []
  command: [payload: DeviceCommandRequest, presetKey?: string]
}>()

const { t } = useI18n()
const { smAndDown } = useDisplay()
const confirmDelete = ref(false)
const renameDraft = ref('')
const activePresetKey = ref('')
const commandDraft = reactive<{
  command: DeviceCommandRequest['command']
  payload: string
  hasParent: boolean
  parentDeviceId: string
}>({
  command: 'custom',
  payload: '',
  hasParent: true,
  parentDeviceId: '',
})

const busy = computed(() => props.busyAction !== null)
const fullscreen = computed(() => smAndDown.value)
const device = computed(() => props.device)
const commandPresets = computed(() => (device.value ? deviceActionPresets(device.value) : []))
const commandOptions = computed(() => [
  { title: t('device.commands.custom'), value: 'custom' },
  { title: t('device.commands.setStatus'), value: 'set_status' },
  { title: t('device.commands.setParent'), value: 'set_parent' },
  { title: t('device.commands.updateConfig'), value: 'update_config' },
])

const statusText = computed(() => {
  if (device.value === null) {
    return ''
  }
  return device.value.status || device.value.lifecycleStatus
})

const statusColor = computed(() => {
  if (device.value === null) {
    return 'primary'
  }
  switch (device.value.status) {
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

const baseFieldRows = computed(() => {
  if (device.value === null) {
    return []
  }
  return [
    { key: 'device-id', label: t('device.fields.deviceId'), value: `#${device.value.deviceId}` },
    { key: 'type', label: t('device.fields.type'), value: device.value.typeLabel },
    { key: 'status', label: t('device.fields.status'), value: statusText.value },
    { key: 'enabled', label: t('device.fields.enabled'), value: yesNo(device.value.enabled) },
    { key: 'registry-revision', label: t('device.fields.registryRevision'), value: `${device.value.registryRevision}` },
    { key: 'config-revision', label: t('device.fields.configRevision'), value: `${device.value.configRevision}` },
    { key: 'pending-persistence', label: t('device.fields.pendingPersistence'), value: yesNo(device.value.pendingPersistence) },
    { key: 'lifecycle', label: t('device.fields.lifecycle'), value: device.value.lifecycleStatus },
    { key: 'effective-status', label: t('device.fields.effectiveStatus'), value: device.value.effectiveStatus },
    { key: 'persistence-policy', label: t('device.fields.persistencePolicy'), value: device.value.persistencePolicy },
    { key: 'parent-device', label: t('device.fields.parentDeviceId'), value: device.value.hasParent ? `#${device.value.parentDeviceId}` : '—' },
    { key: 'has-parent', label: t('device.fields.hasParent'), value: yesNo(device.value.hasParent) },
  ]
})

function yesNo(value: boolean | undefined): string {
  return value === true ? t('labels.yes') : value === false ? t('labels.no') : '—'
}

function resetDrafts(current: DashboardDevice | null): void {
  renameDraft.value = current?.name ?? ''
  commandDraft.command = 'custom'
  commandDraft.payload = ''
  commandDraft.hasParent = true
  commandDraft.parentDeviceId = current?.parentDeviceId ? `${current.parentDeviceId}` : ''
  activePresetKey.value = ''
  confirmDelete.value = false
}

watch(
  () => props.device,
  deviceValue => {
    resetDrafts(deviceValue)
  },
  { immediate: true },
)

watch(
  () => props.busyAction,
  action => {
    if (action !== 'command') {
      activePresetKey.value = ''
    }
  },
)

function submitRename(): void {
  const name = renameDraft.value.trim()
  if (name.length > 0) {
    emit('rename', name)
  }
}

function toggleEnabled(): void {
  if (device.value === null) {
    return
  }
  emit('toggle-enabled', !device.value.enabled)
}

function submitDelete(): void {
  confirmDelete.value = false
  emit('delete')
}

function submitCommand(): void {
  const payload: DeviceCommandRequest = {
    command: commandDraft.command,
  }
  if (commandDraft.command === 'set_parent') {
    payload.has_parent = commandDraft.hasParent
    if (commandDraft.parentDeviceId.trim().length > 0) {
      payload.parent_device_id = Number(commandDraft.parentDeviceId)
    }
  } else if (commandDraft.payload.trim().length > 0) {
    payload.payload = commandDraft.payload.trim()
  }
  emit('command', payload)
}

function runPreset(preset: DashboardDeviceActionPreset): void {
  activePresetKey.value = preset.key
  emit('command', {
    command: preset.command,
    payload: preset.payload,
  }, preset.key)
}
</script>

<style scoped>
.device-dialog {
  border-radius: 24px;
}

.device-dialog__title {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.device-dialog__title-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.device-dialog__icon-button {
  min-width: 40px;
  padding-inline: 8px;
}

.device-dialog__icon-button :deep(.app-icon) {
  width: 18px;
  height: 18px;
}

.device-dialog__eyebrow {
  font-size: 0.72rem;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: #64748b;
}

.device-dialog__headline {
  font-size: clamp(1.2rem, 2vw, 1.7rem);
  font-weight: 700;
  color: #0f172a;
}

.device-dialog__subline {
  margin-top: 4px;
  color: #64748b;
}

.device-dialog__body {
  display: grid;
  gap: 20px;
}

.device-dialog__section {
  display: grid;
  gap: 14px;
}

.device-dialog__section-title {
  font-size: 0.82rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: #475569;
}

.device-dialog__section-copy {
  color: #475569;
}

.device-field {
  display: grid;
  gap: 6px;
  padding: 14px 16px;
  border-radius: 16px;
  background: rgba(248, 250, 252, 0.9);
  border: 1px solid rgba(148, 163, 184, 0.18);
}

.device-field span {
  font-size: 0.78rem;
  color: #64748b;
}

.device-field strong {
  color: #0f172a;
  font-size: 0.96rem;
  word-break: break-word;
}

.device-dialog__action-col {
  display: flex;
  align-items: end;
}

.typed-panel,
.generic-panel {
  display: grid;
  gap: 16px;
  padding: 16px;
  border-radius: 18px;
  background: rgba(248, 250, 252, 0.88);
  border: 1px solid rgba(148, 163, 184, 0.18);
}

.typed-panel__header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}

.typed-panel__title {
  font-weight: 700;
  color: #0f172a;
}

.typed-panel__subtitle {
  color: #64748b;
  font-size: 0.88rem;
}

.typed-panel__grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.typed-panel__commands {
  display: grid;
  gap: 12px;
}

.typed-panel__commands-title {
  font-size: 0.82rem;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: #475569;
}

.typed-panel__command-row {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.generic-panel__actions {
  display: flex;
  justify-content: flex-end;
}

.device-dialog__empty {
  padding: 24px;
  color: #64748b;
}

.device-dialog__footer {
  display: flex;
  align-items: center;
  gap: 12px;
  padding-inline: 16px;
  padding-block: 12px;
}

.device-dialog__error {
  flex: 1 1 auto;
}

@media (max-width: 640px) {
  .device-dialog__title {
    flex-direction: column;
  }

  .typed-panel__grid {
    grid-template-columns: 1fr;
  }
}
</style>
