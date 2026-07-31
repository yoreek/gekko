<template>
  <v-menu v-model="menuOpen" :close-on-content-click="false" @update:model-value="onMenuToggle">
    <template #activator="{ props: activatorProps }">
      <v-text-field
        v-bind="activatorProps"
        :label="label ?? t('device.fields.pixelColor')"
        :model-value="hex"
        density="compact"
        variant="outlined"
        hide-details
        readonly
        :disabled="disabled"
      >
        <template #prepend-inner>
          <v-avatar :color="hex" size="20" />
        </template>
      </v-text-field>
    </template>
    <v-card>
      <v-color-picker
        v-model="stagedHex"
        mode="hex"
        :modes="['hex']"
        :disabled="readonly || disabled"
        flat
      />
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="cancel">{{ t('actions.cancel') }}</v-btn>
        <v-btn variant="tonal" color="primary" @click="apply">{{ t('actions.apply') }}</v-btn>
      </v-card-actions>
    </v-card>
  </v-menu>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import type { PixelColor } from '@/api/contracts'

const props = defineProps<{ modelValue: PixelColor; label?: string; readonly?: boolean; disabled?: boolean }>()
const emit = defineEmits<{ 'update:modelValue': [value: PixelColor] }>()
const { t } = useI18n()

function channelToHex(value: number): string {
  return Math.min(255, Math.max(0, Math.round(value))).toString(16).padStart(2, '0')
}

const hex = computed(
  () => `#${channelToHex(props.modelValue.r)}${channelToHex(props.modelValue.g)}${channelToHex(props.modelValue.b)}`.toUpperCase(),
)

const menuOpen = ref(false)
const stagedHex = ref(hex.value)

function onMenuToggle(open: boolean): void {
  if (open) {
    // Re-seed the staged draft from the live value every time the picker opens, so a previous
    // cancelled edit never leaks into the next session.
    stagedHex.value = hex.value
  }
}

function apply(): void {
  if (!/^#[0-9a-f]{6}$/i.test(stagedHex.value)) {
    menuOpen.value = false
    return
  }
  emit('update:modelValue', {
    r: parseInt(stagedHex.value.slice(1, 3), 16),
    g: parseInt(stagedHex.value.slice(3, 5), 16),
    b: parseInt(stagedHex.value.slice(5, 7), 16),
  })
  menuOpen.value = false
}

function cancel(): void {
  menuOpen.value = false
}
</script>
