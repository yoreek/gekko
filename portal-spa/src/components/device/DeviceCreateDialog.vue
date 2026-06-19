<template>
  <DeviceDialogShell
    :model-value="modelValue"
    :eyebrow="t('device.dialog.createTitle')"
    :subline="t('device.dialog.createHint')"
    :max-width="720"
    @update:model-value="$emit('update:modelValue', $event)"
  >
    <v-alert v-if="errorMessage" type="error" variant="tonal" class="mb-4">
      {{ errorMessage }}
    </v-alert>

    <div class="device-dialog__content">
      <section class="device-dialog__section">
        <DeviceCommonFields
          v-model="draft.common"
          :mode="'create'"
          :busy="loading"
        />
      </section>

      <section v-if="createFormComponent" class="device-dialog__section">
        <div class="device-dialog__section-heading text-overline">{{ t('device.dialog.details') }}</div>
        <component
          :is="createFormComponent"
          v-model="draft.config"
          :busy="loading"
        />
      </section>
    </div>

    <template #footer>
      <v-spacer />
      <v-btn variant="text" @click="$emit('update:modelValue', false)">
        {{ t('actions.cancel') }}
      </v-btn>
      <v-btn color="primary" :loading="loading" :disabled="!canSubmit" @click="submit">
        {{ t('device.dashboard.create') }}
      </v-btn>
    </template>
  </DeviceDialogShell>
</template>

<script setup lang="ts">
import { computed, reactive, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceCommonFields from '@/components/device/DeviceCommonFields.vue'
import DeviceDialogShell from '@/components/device/DeviceDialogShell.vue'
import {
  createDefaultDeviceCommonDraft,
  createDefaultDeviceConfigDraft,
  isDs18b20Type,
  type DeviceCommonDraft,
} from '@/components/device/device-form'
import { resolveDeviceCreateFormComponent } from '@/components/devices/registry/device-component-registry'
import {
  ds18b20AddressShapeValid,
  encodeDs18b20Config,
  normalizeDs18b20TemperatureSensorConfig,
} from '@/models/devices/ds18b20'

type CreatePayload = {
  name: string
  type_id: number
  enabled: boolean
  config?: Record<string, unknown>
  has_parent?: boolean
  parent_device_id?: number
}

const props = defineProps<{
  modelValue: boolean
  loading: boolean
  errorMessage: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  submit: [payload: CreatePayload]
}>()

const { t } = useI18n()

const draft = reactive<{
  common: DeviceCommonDraft
  config: Record<string, unknown>
}>({
  common: createDefaultDeviceCommonDraft(),
  config: {},
})

const ds18b20Config = computed(() => normalizeDs18b20TemperatureSensorConfig(draft.config))
const canSubmit = computed(() => {
  if (draft.common.name.trim().length === 0 || draft.common.typeId <= 0) {
    return false
  }
  if (!isDs18b20Type(draft.common.typeId)) {
    return true
  }
  return ds18b20Config.value.parent_device_id > 0 && ds18b20AddressShapeValid(ds18b20Config.value.address)
})
const createFormComponent = computed(() => resolveDeviceCreateFormComponent(draft.common.typeId))

watch(
  () => props.modelValue,
  value => {
    if (value) {
      draft.common = createDefaultDeviceCommonDraft()
      draft.config = createDefaultDeviceConfigDraft(draft.common.typeId)
    }
  },
)

watch(
  () => draft.common.typeId,
  typeId => {
    draft.config = createDefaultDeviceConfigDraft(typeId)
  },
)

function submit(): void {
  if (!canSubmit.value) {
    return
  }
  const payload: CreatePayload = {
    name: draft.common.name.trim(),
    type_id: draft.common.typeId,
    enabled: draft.common.enabled,
  }
  if (isDs18b20Type(draft.common.typeId)) {
    payload.config = encodeDs18b20Config({
      ...ds18b20Config.value,
      enabled: draft.common.enabled,
    })
    payload.has_parent = true
    payload.parent_device_id = ds18b20Config.value.parent_device_id
  } else if (Object.keys(draft.config).length > 0) {
    payload.config = { ...draft.config }
  }
  emit('submit', payload)
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

.device-dialog__section-heading {
  margin-bottom: 2px;
}
</style>
