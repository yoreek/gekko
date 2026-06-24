<template>
  <DeviceDialogShell
    :model-value="modelValue"
    :eyebrow="t('device.dialog.createTitle')"
    :subline="t('device.dialog.createHint')"
    :max-width="920"
    @update:model-value="$emit('update:modelValue', $event)"
  >
    <v-alert v-if="errorMessage" type="error" variant="tonal" class="mb-4">
      {{ errorMessage }}
    </v-alert>

    <v-form v-model="formValid" class="device-dialog__content">
      <section class="device-dialog__section">
        <DeviceCommonFields
          v-model="draft"
          :mode="'create'"
          :busy="loading"
        />
      </section>

      <section v-if="createFormComponent" class="device-dialog__section">
        <div class="device-dialog__section-heading text-overline">{{ t('device.dialog.details') }}</div>
        <component
          :is="createFormComponent"
          v-model="draft"
          :busy="loading"
        />
      </section>
    </v-form>

    <template #footer>
      <v-spacer />
      <v-btn variant="text" @click="$emit('update:modelValue', false)">
        {{ t('actions.cancel') }}
      </v-btn>
      <v-btn color="primary" :loading="loading" :disabled="!formValid || loading" @click="submit">
        {{ t('device.dashboard.create') }}
      </v-btn>
    </template>
  </DeviceDialogShell>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceCommonFields from '@/components/device/DeviceCommonFields.vue'
import DeviceDialogShell from '@/components/device/DeviceDialogShell.vue'
import {
  createDefaultDeviceDraft,
  type DeviceCreateDraft,
} from '@/components/device/device-form'
import { resolveDeviceFormComponent } from '@/components/devices/registry/device-component-registry'
import { resolveDeviceModelByTypeName } from '@/models/devices/device-model-factory'
import type { DeviceCreatePayload } from '@/models/devices/base-device'

const props = defineProps<{
  modelValue: boolean
  loading: boolean
  errorMessage: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  submit: [payload: DeviceCreatePayload]
}>()

const { t } = useI18n()

const draft = ref<DeviceCreateDraft>(createDefaultDeviceDraft())
const createFormComponent = computed(() => resolveDeviceFormComponent(draft.value.typeName))
const formValid = ref(false)

watch(
  () => props.modelValue,
  value => {
    if (value) {
      replaceDraft(createDefaultDeviceDraft())
    }
  },
)

watch(
  () => draft.value.typeName,
  typeName => {
    formValid.value = false
    const next = createDefaultDeviceDraft(typeName)
    next.name = draft.value.name
    next.enabled = draft.value.enabled
    replaceDraft(next)
  },
)

function submit(): void {
  if (!formValid.value) {
    return
  }
  const model = resolveDeviceModelByTypeName(draft.value.typeName)
  emit('submit', model.buildCreatePayload(draft.value))
}

function replaceDraft(next: DeviceCreateDraft): void {
  draft.value = next
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
