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
      <v-btn variant="text" :disabled="disabled || busy || channelIds.length === 0" @click="openSave">
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
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { SchedulePresetSlot } from '@/api/contracts'
import type { ScheduledAnalogOutputPointDraft } from '@/models/devices/composable-analog-output'
import { deleteSchedulePreset, fetchSchedulePresets, saveSchedulePreset } from '@/api/schedulePresets'
import { presetPointsToDraft, draftPointsToPreset } from '@/models/devices/schedule-preset-points'

// A composer preset is grouped by slot number across its scheduled channels. Each channel owns its
// own slot files; the group name is taken from the first channel that has the slot. Apply loads
// slot N into every channel that has it (channels without it are left unchanged); save writes each
// channel's current schedule into its slot N under one shared name.
const MAX_SLOTS = 3

const props = defineProps<{
  channelIds: number[]
  channelPoints: Record<number, ScheduledAnalogOutputPointDraft[]>
  disabled?: boolean
}>()

const emit = defineEmits<{
  apply: [pointsByChannel: Record<number, ScheduledAnalogOutputPointDraft[]>]
}>()

const { t } = useI18n()

const perChannel = ref<Record<number, SchedulePresetSlot[]>>({})
const busy = ref(false)
const selectedSlot = ref(0)
const saveDialogOpen = ref(false)
const saveName = ref('')

function slotOf(channelId: number, slot: number): SchedulePresetSlot | undefined {
  return perChannel.value[channelId]?.find(entry => entry.slot === slot)
}

const groupSlots = computed(() =>
  Array.from({ length: MAX_SLOTS }, (_unused, slot) => {
    const namedChannel = props.channelIds.find(id => slotOf(id, slot)?.filled)
    return {
      slot,
      filled: namedChannel !== undefined,
      name: namedChannel !== undefined ? (slotOf(namedChannel, slot)?.name ?? '') : '',
    }
  }),
)
const slotItems = computed(() =>
  groupSlots.value.map(entry => ({
    title: entry.filled ? entry.name : t('device.dialog.schedulePresets.emptySlot', { slot: entry.slot + 1 }),
    value: entry.slot,
  })),
)
const selectedFilled = computed(() => groupSlots.value[selectedSlot.value]?.filled === true)

async function reload(): Promise<void> {
  const ids = props.channelIds
  if (ids.length === 0) {
    perChannel.value = {}
    return
  }
  busy.value = true
  try {
    const responses = await Promise.all(ids.map(id => fetchSchedulePresets(id)))
    const next: Record<number, SchedulePresetSlot[]> = {}
    ids.forEach((id, index) => {
      next[id] = responses[index].presets
    })
    perChannel.value = next
  } finally {
    busy.value = false
  }
}

watch(() => props.channelIds.join(','), reload, { immediate: true })

function applySelected(): void {
  const slot = selectedSlot.value
  const map: Record<number, ScheduledAnalogOutputPointDraft[]> = {}
  for (const id of props.channelIds) {
    const preset = slotOf(id, slot)
    if (preset?.filled) {
      map[id] = presetPointsToDraft(preset.points ?? [])
    }
  }
  emit('apply', map)
}

function openSave(): void {
  saveName.value = groupSlots.value[selectedSlot.value]?.name ?? ''
  saveDialogOpen.value = true
}

async function confirmSave(): Promise<void> {
  const name = saveName.value.trim()
  if (!name || busy.value) {
    return
  }
  const slot = selectedSlot.value
  busy.value = true
  try {
    await Promise.all(
      props.channelIds.map(id => saveSchedulePreset(id, slot, name, draftPointsToPreset(props.channelPoints[id] ?? []))),
    )
    saveDialogOpen.value = false
  } finally {
    busy.value = false
  }
  await reload()
}

async function removeSelected(): Promise<void> {
  const slot = selectedSlot.value
  busy.value = true
  try {
    await Promise.all(props.channelIds.filter(id => slotOf(id, slot)?.filled).map(id => deleteSchedulePreset(id, slot)))
  } finally {
    busy.value = false
  }
  await reload()
}
</script>
