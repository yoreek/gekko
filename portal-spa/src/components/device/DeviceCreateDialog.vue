<template>
  <v-dialog
    :model-value="modelValue"
    max-width="720"
    scrollable
    @update:model-value="$emit('update:modelValue', $event)"
  >
    <v-card class="device-dialog">
      <v-card-title class="device-dialog__title">
        <div>
          <div class="device-dialog__eyebrow">{{ t('device.dialog.createTitle') }}</div>
          <div class="device-dialog__subline">
            {{ t('device.dialog.createHint') }}
          </div>
        </div>
        <v-btn class="device-dialog__icon-button" variant="text" @click="$emit('update:modelValue', false)">
          <AppIcon name="close" />
        </v-btn>
      </v-card-title>

      <v-divider />

      <v-card-text class="device-dialog__body">
        <v-alert v-if="errorMessage" type="error" variant="tonal" density="compact" class="mb-4">
          {{ errorMessage }}
        </v-alert>

        <v-row dense>
          <v-col cols="12" md="8">
            <v-text-field
              v-model="draft.name"
              :label="t('device.actions.name')"
              density="comfortable"
              hide-details
            />
          </v-col>
          <v-col cols="12" md="4">
            <v-select
              v-model="draft.type_id"
              :items="typeOptions"
              :label="t('device.actions.type')"
              density="comfortable"
              hide-details
            />
          </v-col>
          <v-col cols="12" md="4">
            <v-switch
              v-model="draft.enabled"
              :label="t('device.fields.enabled')"
              hide-details
              inset
            />
          </v-col>
        </v-row>
      </v-card-text>

      <v-divider />

      <v-card-actions class="device-dialog__footer">
        <v-spacer />
        <v-btn variant="text" @click="$emit('update:modelValue', false)">
          {{ t('actions.cancel') }}
        </v-btn>
        <v-btn color="primary" :loading="loading" :disabled="!canSubmit" @click="submit">
          {{ t('device.dashboard.create') }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed, reactive, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import AppIcon from '@/components/AppIcon.vue'
import { DUMMY_DEVICE_TYPE_ID, deviceTypeOptions } from '@/models/device-types'

type CreatePayload = {
  name: string
  type_id: number
  enabled: boolean
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

const typeOptions = computed(() => deviceTypeOptions.map(option => ({ title: t(option.labelKey), value: option.id })))
const draft = reactive<CreatePayload>({
  name: 'New Device',
  type_id: DUMMY_DEVICE_TYPE_ID,
  enabled: true,
})

const canSubmit = computed(() => draft.name.trim().length > 0 && draft.type_id > 0)

function resetDraft(): void {
  draft.name = 'New Device'
  draft.type_id = DUMMY_DEVICE_TYPE_ID
  draft.enabled = true
}

function submit(): void {
  if (!canSubmit.value) {
    return
  }
  emit('submit', {
    name: draft.name.trim(),
    type_id: draft.type_id,
    enabled: draft.enabled,
  })
}

watch(
  () => props.modelValue,
  value => {
    if (value) {
      resetDraft()
    }
  }
)
</script>
