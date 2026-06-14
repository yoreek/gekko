<template>
  <v-dialog
    :model-value="modelValue"
    :max-width="maxWidth"
    scrollable
    :fullscreen="fullscreen"
    @update:model-value="$emit('update:modelValue', $event)"
  >
    <v-card class="device-dialog">
      <v-card-title class="device-dialog__title">
        <div>
          <div v-if="eyebrow" class="device-dialog__eyebrow">{{ eyebrow }}</div>
          <div v-if="headline" class="device-dialog__headline">{{ headline }}</div>
          <div v-if="subline" class="device-dialog__subline">{{ subline }}</div>
        </div>
        <div class="device-dialog__title-actions">
          <slot name="title-actions" />
          <v-btn
            class="device-dialog__icon-button"
            variant="text"
            :aria-label="t('device.actions.close')"
            @click="$emit('update:modelValue', false)"
          >
            <AppIcon name="close" />
          </v-btn>
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

import AppIcon from '@/components/AppIcon.vue'

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
