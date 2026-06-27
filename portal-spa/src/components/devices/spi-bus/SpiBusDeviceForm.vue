<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.spiHost')"
            :items="hostItems"
            :model-value="currentValue.host"
            :disabled="busy"
            @update:model-value="updateNumber('host', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.spiSckPin')"
            :model-value="currentValue.sckPin"
            type="number"
            inputmode="numeric"
            :disabled="busy"
            @update:model-value="updateNumber('sckPin', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.spiMosiPin')"
            :model-value="currentValue.mosiPin"
            type="number"
            inputmode="numeric"
            :disabled="busy"
            @update:model-value="updateNumber('mosiPin', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.spiMisoPin')"
            :model-value="currentValue.misoPin"
            type="number"
            inputmode="numeric"
            :disabled="busy"
            @update:model-value="updateNumber('misoPin', $event)"
          />
        </v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { defaultSpiBusConfig, type SpiBusConfigDraft, type SpiBusCreateDraft } from '@/models/devices/spi-bus'

type SpiBusFormValue = SpiBusCreateDraft | SpiBusConfigDraft

const props = defineProps<{
  modelValue?: SpiBusFormValue
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: SpiBusFormValue]
}>()

const { t } = useI18n()
const fallbackValue: SpiBusFormValue = {
  ...defaultSpiBusConfig(),
}
const currentValue = computed<SpiBusFormValue>(() => props.modelValue ?? fallbackValue)
const hostItems = [
  { title: t('device.dialog.spiBus.hosts.hspi'), value: 1 },
  { title: t('device.dialog.spiBus.hosts.vspi'), value: 2 },
]

function updateNumber<K extends keyof SpiBusCreateDraft>(key: K, value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  emit('update:modelValue', {
    ...(currentValue.value as SpiBusCreateDraft),
    [key]: numeric,
  } as SpiBusFormValue)
}
</script>
