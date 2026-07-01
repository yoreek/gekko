<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.gpioPin')"
            :hint="t('device.dialog.common.onewirePinHint')"
            persistent-hint
            :model-value="currentValue.gpioPin"
            inputmode="numeric"
            type="number"
            :disabled="busy"
            @update:model-value="updatePin"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-switch
            :label="t('device.fields.internalPullup')"
            :model-value="currentValue.internalPullup"
            :disabled="busy"
            inset
            @update:model-value="update('internalPullup', Boolean($event))"
          />
        </v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { OneWireBusConfigDraft, OneWireBusCreateDraft } from '@/models/devices/onewire-bus'

type OneWireBusFormValue = OneWireBusCreateDraft | OneWireBusConfigDraft

const props = defineProps<{
  modelValue: OneWireBusFormValue | undefined
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: OneWireBusFormValue]
}>()

const { t } = useI18n()
const isCreateMode = computed(() => props.mode !== 'edit')
const fallbackValue: OneWireBusFormValue = {
  name: 'New Device',
  enabled: true,
  deps: [],
  gpioPin: 4,
  internalPullup: false,
}
const currentValue = computed<OneWireBusFormValue>(() => props.modelValue ?? fallbackValue)

function updatePin(value: string | number): void {
  const gpioPin = Number(value)
  emit('update:modelValue', buildNextValue({ gpioPin: Number.isFinite(gpioPin) ? gpioPin : currentValue.value.gpioPin }))
}

function update<K extends keyof OneWireBusCreateDraft>(key: K, value: OneWireBusCreateDraft[K]): void {
  emit('update:modelValue', buildNextValue({ [key]: value } as Partial<OneWireBusCreateDraft>))
}

function buildNextValue(patch: Partial<OneWireBusCreateDraft>): OneWireBusFormValue {
  if (!isCreateMode.value) {
    return {
      ...(currentValue.value as OneWireBusCreateDraft),
      ...patch,
    }
  }
  return {
    ...(currentValue.value as OneWireBusCreateDraft),
    ...patch,
  }
}
</script>

<style scoped>
.device-type-stack {
  display: grid;
  gap: 12px;
}

.device-type-section {
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 10px;
  background: var(--portal-surface);
  box-shadow: var(--portal-shadow-sm);
}

.device-type-section__grid {
  margin: 0;
}
</style>
