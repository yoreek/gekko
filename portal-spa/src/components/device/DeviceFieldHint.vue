<template>
  <v-tooltip :text="text" location="bottom" open-on-click>
    <template #activator="{ props: tooltipProps }">
      <v-btn
        v-bind="tooltipProps"
        class="device-field-hint"
        :aria-label="ariaLabel"
        icon
        size="x-small"
        color="on-surface"
        variant="text"
      >
        <AppIcon name="info" />
      </v-btn>
    </template>
  </v-tooltip>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import AppIcon from '@/components/AppIcon.vue'

defineProps<{
  text: string
}>()

const { t } = useI18n()
const ariaLabel = computed(() => t('device.dialog.fieldHint'))
</script>
