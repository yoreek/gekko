<template>
  <v-dialog
    :model-value="modelValue"
    :max-width="maxWidth"
    :fullscreen="fullscreen"
    @update:model-value="$emit('update:modelValue', $event)"
  >
    <v-card class="device-dialog">
      <v-card-title class="device-dialog__title">
        <div class="text-wrap">
          <div v-if="eyebrow" class="text-overline">{{ eyebrow }}</div>
          <div v-if="headline" class="text-h6 font-weight-bold">{{ headline }}</div>
          <div v-if="subline" class="text-body-2 text-medium-emphasis">{{ subline }}</div>
        </div>
        <div class="device-dialog__title-actions">
          <slot name="title-actions" />
          <v-btn
            class="device-dialog__icon-button"
            icon="close"
            variant="text"
            :aria-label="t('device.actions.close')"
            @click="$emit('update:modelValue', false)"
          />
        </div>
      </v-card-title>

      <v-divider />

      <v-card-text class="device-dialog__body">
        <slot />
      </v-card-text>

      <v-divider />

      <v-card-actions class="device-dialog__footer">
        <slot name="footer" />
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

defineProps<{
  modelValue: boolean
  eyebrow?: string
  headline?: string
  subline?: string
  fullscreen?: boolean
  maxWidth?: number | string
}>()

defineEmits<{
  'update:modelValue': [value: boolean]
}>()

const { t } = useI18n()
</script>

<style scoped>
.device-dialog {
  overflow: hidden;
}

.device-dialog__title {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
  padding: 18px 20px 14px;
  flex-wrap: wrap;
}

.device-dialog__title-actions {
  display: flex;
  align-items: center;
  gap: 8px;
  flex: 0 1 auto;
  flex-wrap: wrap;
  justify-content: flex-end;
}

.device-dialog__icon-button {
  min-width: 36px;
  width: 36px;
  height: 36px;
}

.device-dialog__body {
  padding: 14px 16px 16px;
}

.device-dialog__footer {
  padding: 12px 16px 14px;
}
</style>
