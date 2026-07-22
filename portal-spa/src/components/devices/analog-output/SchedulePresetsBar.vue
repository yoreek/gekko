<template>
  <div>
    <div class="text-body-medium text-medium-emphasis mb-2">{{ t('device.dialog.schedulePresets.title') }}</div>
    <v-row density="comfortable">
      <v-col v-for="preset in presets" :key="preset.slot" cols="12" sm="4">
        <v-card variant="tonal" class="pa-3 d-flex flex-column ga-2">
          <span class="text-body-2 text-truncate">
            {{ preset.filled ? preset.name : t('device.dialog.schedulePresets.emptySlot', { slot: preset.slot + 1 }) }}
          </span>
          <div class="d-flex flex-wrap ga-1">
            <template v-if="preset.filled">
              <v-btn size="small" variant="tonal" :disabled="disabled || busy" @click="apply(preset)">
                {{ t('device.dialog.schedulePresets.apply') }}
              </v-btn>
              <v-btn size="small" variant="text" :disabled="disabled || busy" @click="openSave(preset.slot, preset.name)">
                {{ t('device.dialog.schedulePresets.overwrite') }}
              </v-btn>
              <v-btn size="small" variant="text" color="error" :disabled="disabled || busy" @click="remove(preset.slot)">
                {{ t('device.dialog.schedulePresets.delete') }}
              </v-btn>
            </template>
            <v-btn
              v-else
              size="small"
              variant="tonal"
              :disabled="disabled || busy || points.length === 0"
              @click="openSave(preset.slot)"
            >
              {{ t('device.dialog.schedulePresets.save') }}
            </v-btn>
          </div>
        </v-card>
      </v-col>
    </v-row>

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
import { onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { SchedulePresetPoint, SchedulePresetSlot } from '@/api/contracts'
import type { ScheduledAnalogOutputPointDraft } from '@/models/devices/composable-analog-output'
import { deleteSchedulePreset, fetchSchedulePresets, saveSchedulePreset } from '@/api/schedulePresets'

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
const saveDialogOpen = ref(false)
const saveName = ref('')
const saveSlot = ref(0)

async function reload(): Promise<void> {
  if (!(props.deviceId > 0)) {
    presets.value = []
    return
  }
  busy.value = true
  try {
    const response = await fetchSchedulePresets(props.deviceId)
    presets.value = response.presets
  } finally {
    busy.value = false
  }
}

onMounted(reload)
watch(() => props.deviceId, reload)

function apply(preset: SchedulePresetSlot): void {
  const points = (preset.points ?? []).map(point => ({ deleted: false, minuteOfDay: point.minuteOfDay, state: point.state }))
  emit('apply', points)
}

function openSave(slot: number, existingName = ''): void {
  saveSlot.value = slot
  saveName.value = existingName
  saveDialogOpen.value = true
}

async function confirmSave(): Promise<void> {
  const name = saveName.value.trim()
  if (!name || busy.value) {
    return
  }
  const points: SchedulePresetPoint[] = props.points
    .filter(point => !point.deleted)
    .map(point => ({ minuteOfDay: point.minuteOfDay, state: point.state }))
  busy.value = true
  try {
    await saveSchedulePreset(props.deviceId, saveSlot.value, name, points)
    saveDialogOpen.value = false
  } finally {
    busy.value = false
  }
  await reload()
}

async function remove(slot: number): Promise<void> {
  busy.value = true
  try {
    await deleteSchedulePreset(props.deviceId, slot)
  } finally {
    busy.value = false
  }
  await reload()
}
</script>
