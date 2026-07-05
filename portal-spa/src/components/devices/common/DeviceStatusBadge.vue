<template>
  <v-tooltip :text="label" location="top">
    <template #activator="{ props: tooltipProps }">
      <v-btn
        v-bind="tooltipProps"
        :color="color"
        size="small"
        variant="tonal"
        :ripple="false"
        @click.stop
      >
        <v-icon :icon="icon" size="16" />
      </v-btn>
    </template>
  </v-tooltip>
</template>

<script setup lang="ts">
import type { PortalIconName } from '@/icons'

defineProps<{
  icon: PortalIconName
  color: string
  label: string
}>()
</script>
