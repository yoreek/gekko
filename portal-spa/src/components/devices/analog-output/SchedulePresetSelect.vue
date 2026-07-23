<template>
  <div class="d-flex flex-column ga-2">
    <div class="d-flex ga-2 align-center flex-wrap">
      <v-select
        class="flex-grow-1"
        :label="t('device.dialog.schedulePresets.title')"
        :items="slotItems"
        :model-value="selectedSlot"
        :disabled="disabled || busy"
        density="comfortable"
        hide-details
        @update:model-value="selectedSlot = Number($event)"
      />
      <v-btn variant="tonal" :disabled="disabled || busy || !selectedFilled" @click="applySelected">
        {{ t('device.dialog.schedulePresets.apply') }}
      </v-btn>
      <v-btn variant="text" :disabled="disabled || busy || points.length === 0" @click="openSave">
        {{ t('device.dialog.schedulePresets.saveToSlot') }}
      </v-btn>
      <v-btn variant="text" color="error" :disabled="disabled || busy || !selectedFilled" @click="removeSelected">
        {{ t('device.dialog.schedulePresets.delete') }}
      </v-btn>
    </div>

    <v-dialog v-model="saveDialogOpen" max-width="400">
      <v-card>
        <v-card-title>{{ t('device.dialog.schedulePresets.saveTitle') }}</v-card-title>
        <v-card-text>
          <v-text-field
            v-model="saveName"
            :label="t('device.dialog.schedulePresets.name')"
            maxlength="32"
            counter
            autofocus
            @keyup.enter="confirmSave"
          />
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="saveDialogOpen = false">{{ t('actions.cancel') }}</v-btn>
          <v-btn color="primary" variant="flat" :disabled="busy || !saveName.trim()" @click="confirmSave">
            {{ t('device.dialog.schedulePresets.save') }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { SchedulePresetSlot } from '@/api/contracts'
import type { ScheduledAnalogOutputPointDraft } from '@/models/devices/composable-analog-output'
import { deleteSchedulePreset, fetchSchedulePresets, saveSchedulePreset } from '@/api/schedulePresets'
import { presetPointsToDraft, draftPointsToPreset } from '@/models/devices/schedule-preset-points'

const props = defineProps<{
  deviceId: number
  points: ScheduledAnalogOutputPointDraft[]
  disabled?: boolean
}>()

const emit = defineEmits<{
  apply: [points: ScheduledAnalogOutputPointDraft[]]
}>()

const { t } = useI18n()

const presets = ref<SchedulePresetSlot[]>([])
const busy = ref(false)
const selectedSlot = ref(0)
const saveDialogOpen = ref(false)
const saveName = ref('')

const slotItems = computed(() =>
  presets.value.map(preset => ({
    title: preset.filled ? (preset.name ?? '') : t('device.dialog.schedulePresets.emptySlot', { slot: preset.slot + 1 }),
    value: preset.slot,
  })),
)
const selectedPreset = computed(() => presets.value.find(preset => preset.slot === selectedSlot.value))
const selectedFilled = computed(() => selectedPreset.value?.filled === true)

async function reload(): Promise<void> {
  if (!(props.deviceId > 0)) {
    presets.value = []
    return
  }
  busy.value = true
  try {
    presets.value = (await fetchSchedulePresets(props.deviceId)).presets
  } finally {
    busy.value = false
  }
}

onMounted(reload)
watch(() => props.deviceId, reload)

function applySelected(): void {
  if (selectedPreset.value?.filled) {
    emit('apply', presetPointsToDraft(selectedPreset.value.points ?? []))
  }
}

function openSave(): void {
  saveName.value = selectedPreset.value?.name ?? ''
  saveDialogOpen.value = true
}

async function confirmSave(): Promise<void> {
  const name = saveName.value.trim()
  if (!name || busy.value) {
    return
  }
  busy.value = true
  try {
    await saveSchedulePreset(props.deviceId, selectedSlot.value, name, draftPointsToPreset(props.points))
    saveDialogOpen.value = false
  } finally {
    busy.value = false
  }
  await reload()
}

async function removeSelected(): Promise<void> {
  busy.value = true
  try {
    await deleteSchedulePreset(props.deviceId, selectedSlot.value)
  } finally {
    busy.value = false
  }
  await reload()
}
</script>
