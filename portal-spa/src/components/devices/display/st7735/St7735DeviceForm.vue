<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.st7735Display.noDependency') }}
      </v-alert>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.spiBusDeviceId')"
            :items="dependencyItems"
            :model-value="currentValue.spiBusDeviceId"
            :disabled="busy || dependencyItems.length === 0"
            :rules="dependencyRules"
            @update:model-value="updateNumber('spiBusDeviceId', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.chipSelectPin')"
            :model-value="currentValue.chipSelectPin"
            :disabled="busy"
            inputmode="numeric"
            type="number"
            min="0"
            max="39"
            @update:model-value="updateNumber('chipSelectPin', $event)"
          />
        </v-col>
      </v-row>

      <SpiChipSelectProbe
        :bus-device-id="currentValue.spiBusDeviceId"
        :cs-pin="currentValue.chipSelectPin"
        :disabled="busy"
      />

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.dcPin')"
            :model-value="currentValue.dcPin"
            :disabled="busy"
            inputmode="numeric"
            type="number"
            min="0"
            max="39"
            @update:model-value="updateNumber('dcPin', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.resetPin')"
            :model-value="currentValue.resetPin"
            :disabled="busy"
            inputmode="numeric"
            type="number"
            min="-1"
            max="39"
            @update:model-value="updateNumber('resetPin', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.display.orientation')"
            :items="orientationItems"
            :model-value="currentValue.rotation"
            :disabled="busy"
            density="compact"
            variant="outlined"
            hide-details
            @update:model-value="updateNumber('rotation', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.display.width')"
            :model-value="currentValue.width"
            :disabled="busy"
            inputmode="numeric"
            type="number"
            min="1"
            @update:model-value="updateNumber('width', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.display.height')"
            :model-value="currentValue.height"
            :disabled="busy"
            inputmode="numeric"
            type="number"
            min="1"
            @update:model-value="updateNumber('height', $event)"
          />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <div class="text-subtitle-2">{{ t('device.fields.display.layout') }}</div>
      <div class="text-body-2">{{ t('device.dialog.st7735.layoutHint') }}</div>
      <St7735LayoutPreview
        :layout="currentValue.layout"
        :display="st7735Display"
        :device-width="effectiveSize.effectiveWidth"
        :device-height="effectiveSize.effectiveHeight"
        :metric-catalog="metricCatalog"
      />
      <div v-if="mode === 'edit'" class="d-flex justify-end">
        <v-btn variant="text" color="primary" :disabled="busy" @click="emit('design-display')">
          <v-icon class="me-1" icon="design-display" />
          {{ t('device.dialog.st7735Display.designDisplay') }}
        </v-btn>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import SpiChipSelectProbe from '@/components/devices/common/SpiChipSelectProbe.vue'
import St7735LayoutPreview from '@/components/devices/display/st7735/St7735LayoutPreview.vue'
import { useMetricPlaceholderCatalog } from '@/composables/display/useMetricPlaceholderCatalog'
import { SPI_BUS_DEVICE_TYPE_ID, deviceTypeIdFromName } from '@/models/device-type-ids'
import { st7735Display } from '@/models/devices/display/display'
import { resolveDisplayEffectiveSize } from '@/models/devices/display/orientation'
import { St7735Device, type St7735ConfigDraft, type St7735CreateDraft } from '@/models/devices/st7735/device'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

type FormValue = St7735CreateDraft | St7735ConfigDraft

const props = defineProps<{ modelValue?: FormValue; busy?: boolean; mode?: 'create' | 'edit' }>()
const emit = defineEmits<{
  'update:modelValue': [value: FormValue]
  'design-display': []
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const { metricCatalog, refreshMetricCatalog } = useMetricPlaceholderCatalog()
const fallbackValue: St7735CreateDraft = {
  ...St7735Device.defaultConfig(),
  typeName: 'st7735',
}
const currentValue = computed<FormValue>(() => props.modelValue ?? fallbackValue)
const effectiveSize = computed(() => resolveDisplayEffectiveSize(currentValue.value.width, currentValue.value.height, currentValue.value.rotation))
const dependencyDevices = computed(() => deviceStore.devices.filter(device => deviceTypeIdFromName(device.record.typeName) === SPI_BUS_DEVICE_TYPE_ID))
const dependencyItems = computed(() => dependencyDevices.value.map(device => ({ title: `${device.config.name} #${device.record.id}`, value: device.record.id })))
const orientationItems = computed(() => {
  const isWidePanel = currentValue.value.width >= currentValue.value.height
  const naturalLabel = isWidePanel ? t('device.fields.display.orientationLandscape') : t('device.fields.display.orientationPortrait')
  const rotatedLabel = isWidePanel ? t('device.fields.display.orientationPortrait') : t('device.fields.display.orientationLandscape')
  return [
    { title: naturalLabel, value: 0 },
    { title: rotatedLabel, value: 1 },
  ]
})
const dependencyRules = computed(() => [
  (value: unknown) => Number(value) > 0 || t('device.dialog.st7735Display.noDependency'),
])

onMounted(() => {
  void refreshMetricCatalog()
})

function updateNumber(
  key: keyof Pick<St7735CreateDraft, 'spiBusDeviceId' | 'chipSelectPin' | 'dcPin' | 'resetPin' | 'rotation' | 'width' | 'height'>,
  value: string | number,
): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  emit('update:modelValue', { ...fallbackValue, ...(currentValue.value as Partial<St7735CreateDraft>), [key]: numeric } as FormValue)
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
