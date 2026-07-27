<template>
  <v-row density="comfortable">
    <v-col v-if="$slots.notice" cols="12">
      <slot name="notice" />
    </v-col>

    <slot />

    <v-col cols="12">
      <slot name="preview">
        <DisplayStaticPreview
          v-if="previewKind"
          :kind="previewKind"
          :rotation="previewRotation"
          :max-side="previewMaxSide"
        />
      </slot>
    </v-col>

    <v-col v-if="device" cols="12">
      <v-btn
        color="primary"
        variant="tonal"
        :to="{ name: 'device-design', params: { id: device.record.id } }"
      >
        <v-icon class="me-2" icon="design-display" />
        {{ designerLabel }}
      </v-btn>
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import type { DeviceRecord } from '@/api/contracts'
import DisplayStaticPreview from '@/components/devices/display/DisplayStaticPreview.vue'

withDefaults(defineProps<{
  device?: DeviceRecord
  previewKind?: 'lcd1602' | 'lcd2004' | 'tm1637'
  previewRotation?: number
  previewMaxSide?: number
  designerLabel: string
}>(), {
  previewRotation: 0,
  previewMaxSide: 180,
})
</script>
