<template>
  <div class="d-flex flex-column ga-4">
    <div v-if="modelValue.rules.length > 0" class="d-flex flex-column ga-1">
      <div class="d-flex align-center flex-wrap ga-2">
        <v-chip :color="statusPreview.active ? 'success' : 'secondary'" variant="tonal">
          {{ statusPreview.active ? t('device.dialog.schedule.currentlyOn') : t('device.dialog.schedule.currentlyOff') }}
        </v-chip>
        <span v-if="nextChangeText" class="text-body-small text-medium-emphasis">{{ nextChangeText }}</span>
      </div>
      <div class="text-body-small text-medium-emphasis">{{ t('device.dialog.schedule.estimateNote') }}</div>
    </div>

    <v-alert v-if="modelValue.rules.length === 0" type="info" variant="tonal">
      {{ t('device.dialog.schedule.noRules') }}
    </v-alert>

    <v-card v-for="(rule, index) in modelValue.rules" :key="index" variant="outlined" color="on-surface" class="pa-3">
      <div class="d-flex align-center justify-space-between mb-2">
        <div class="text-label-large">{{ t('device.dialog.schedule.ruleTitle', { n: index + 1 }) }}</div>
        <div class="d-flex align-center ga-2">
          <v-switch
            :model-value="rule.enabled"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            density="comfortable"
            hide-details
            inset
            @update:model-value="updateRule(index, { enabled: Boolean($event) })"
          />
          <v-btn
            v-if="mode !== 'view'"
            icon="trash"
            variant="text"
            density="comfortable"
            :disabled="busy"
            @click="removeRule(index)"
          />
        </div>
      </div>

      <div class="d-flex align-center flex-wrap ga-1 mb-3">
        <v-checkbox
          v-for="day in weekDayOptions"
          :key="day.value"
          :model-value="rule.weekDays.includes(day.value)"
          :label="day.label"
          density="compact"
          hide-details
          :disabled="(busy && mode !== 'view') || mode === 'view'"
          @update:model-value="setWeekday(index, day.value, Boolean($event))"
        />
        <v-btn
          size="small"
          variant="text"
          :disabled="(busy && mode !== 'view') || mode === 'view'"
          @click="toggleAllWeekdays(index)"
        >
          {{ rule.weekDays.length === 7 ? t('device.dialog.schedule.deselectAllDays') : t('device.dialog.schedule.selectAllDays') }}
        </v-btn>
      </div>

      <v-row>
        <v-col cols="6" sm="3">
          <v-text-field
            type="time"
            :label="t('device.fields.scheduleStart')"
            :model-value="minutesToTimeString(rule.startMinuteOfDay)"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="updateRule(index, { startMinuteOfDay: timeStringToMinutes($event) })"
          />
        </v-col>
        <v-col cols="6" sm="3">
          <v-text-field
            type="time"
            :label="t('device.fields.scheduleEnd')"
            :model-value="minutesToTimeString(rule.endMinuteOfDay)"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="updateRule(index, { endMinuteOfDay: timeStringToMinutes($event) })"
          />
        </v-col>
        <v-col cols="12" sm="6">
          <v-select
            :label="t('device.fields.scheduleMode')"
            :items="modeItems"
            :model-value="rule.mode"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="updateRule(index, { mode: $event as ScheduleRuleMode })"
          />
        </v-col>
      </v-row>

      <v-row v-if="rule.mode === 'interval'">
        <v-col cols="6">
          <v-text-field
            type="number"
            min="1"
            :label="t('device.fields.scheduleIntervalsPerWindow')"
            :hint="t('device.dialog.schedule.intervalsPerWindowHint')"
            persistent-hint
            :model-value="rule.intervalsPerWindow"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="updateRule(index, { intervalsPerWindow: Number($event) })"
          />
        </v-col>
        <v-col cols="6">
          <v-text-field
            type="number"
            min="1"
            :label="t('device.fields.scheduleDurationMinutes')"
            :hint="t('device.dialog.schedule.durationMinutesHint')"
            persistent-hint
            :model-value="rule.durationMinutes"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="updateRule(index, { durationMinutes: Number($event) })"
          />
        </v-col>
      </v-row>

      <v-alert
        v-if="rule.mode === 'interval' && intervalDurationExceedsSlice(rule)"
        type="warning"
        variant="tonal"
        density="compact"
        class="mt-3"
      >
        {{ t('device.dialog.schedule.intervalDurationExceedsSliceWarning') }}
      </v-alert>

      <div v-if="rule.mode === 'interval'" class="d-flex flex-column ga-1 mt-3">
        <div class="text-body-small text-medium-emphasis">{{ t('device.dialog.schedule.intervalPreviewLabel') }}</div>
        <div class="d-flex flex-wrap ga-2">
          <v-chip v-for="(segment, segIndex) in intervalSegments(rule)" :key="segIndex" size="small" variant="tonal" color="primary">
            {{ minutesToTimeString(segment.sliceStartMinuteOfDay) }}–{{ minutesToTimeString(segment.onEndMinuteOfDay) }}
          </v-chip>
        </div>
      </div>
    </v-card>

    <v-btn
      v-if="mode !== 'view' && modelValue.rules.length < kMaxScheduleRules"
      variant="tonal"
      prepend-icon="plus"
      @click="addRule"
    >
      {{ t('device.dialog.schedule.addRule') }}
    </v-btn>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, ScheduleRuleConfig, ScheduleRuleMode } from '@/api/contracts'
import { type ScheduleConfigDraft, defaultScheduleRule, kMaxScheduleRules } from '@/models/devices/schedule'
import { describeScheduleStatus, intervalDurationExceedsSlice, intervalSegmentStarts } from '@/models/devices/schedule-preview'

const props = defineProps<{
  modelValue: ScheduleConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: ScheduleConfigDraft]
}>()

const { t } = useI18n()

const weekDayOptions = [0, 1, 2, 3, 4, 5, 6].map(value => ({
  value,
  label: t(`device.dialog.schedule.weekdayShort.${value}`),
}))
const modeItems = (['alwaysOn', 'interval'] as ScheduleRuleMode[]).map(value => ({
  title: t(`device.dialog.schedule.mode.${value}`),
  value,
}))

// The firmware never pushes a live "active" value (ScheduleDevice never marks itself
// runtime-dirty), so this is computed client-side from the rule config against the browser's own
// clock instead of read from the server - see schedule-preview.ts. Refreshed once a minute so it
// doesn't need a reactive framework-level clock; a stale-by-a-few-seconds display is fine here.
const now = ref(new Date())
let clockTimer: ReturnType<typeof setInterval> | undefined

onMounted(() => {
  clockTimer = setInterval(() => {
    now.value = new Date()
  }, 30000)
})

onUnmounted(() => {
  if (clockTimer !== undefined) {
    clearInterval(clockTimer)
  }
})

const statusPreview = computed(() => describeScheduleStatus(props.modelValue.rules, now.value))

function formatNextChange(at: Date): string {
  const timeText = `${String(at.getHours()).padStart(2, '0')}:${String(at.getMinutes()).padStart(2, '0')}`
  const sameDay = at.getFullYear() === now.value.getFullYear() && at.getMonth() === now.value.getMonth() && at.getDate() === now.value.getDate()
  if (sameDay) {
    return timeText
  }
  return `${t(`device.dialog.schedule.weekdayShort.${at.getDay()}`)} ${timeText}`
}

const nextChangeText = computed(() => {
  const { active: isActive, nextChangeAt } = statusPreview.value
  if (!nextChangeAt) {
    return ''
  }
  const label = t(isActive ? 'device.dialog.schedule.turnsOffAt' : 'device.dialog.schedule.turnsOnAt', { time: formatNextChange(nextChangeAt) })
  return label
})

// Purely a function of the rule's own time/interval fields (no clock dependency, unlike
// statusPreview above) - shows how the window is sliced, weekday-independent.
function intervalSegments(rule: ScheduleRuleConfig) {
  return intervalSegmentStarts(rule)
}

function minutesToTimeString(minuteOfDay: number): string {
  const hours = Math.floor(minuteOfDay / 60) % 24
  const minutes = minuteOfDay % 60
  return `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}`
}

function timeStringToMinutes(value: string): number {
  const [hoursText, minutesText] = value.split(':')
  const hours = Number(hoursText)
  const minutes = Number(minutesText)
  if (!Number.isFinite(hours) || !Number.isFinite(minutes)) {
    return 0
  }
  return hours * 60 + minutes
}

function updateRule(index: number, patch: Partial<ScheduleRuleConfig>): void {
  const rules = props.modelValue.rules.map((rule, ruleIndex) => (ruleIndex === index ? { ...rule, ...patch } : rule))
  emit('update:modelValue', { ...props.modelValue, rules })
}

function setWeekday(index: number, day: number, checked: boolean): void {
  const current = props.modelValue.rules[index].weekDays
  const weekDays = checked ? [...current, day].sort((a, b) => a - b) : current.filter(value => value !== day)
  updateRule(index, { weekDays })
}

function toggleAllWeekdays(index: number): void {
  const allSelected = props.modelValue.rules[index].weekDays.length === 7
  updateRule(index, { weekDays: allSelected ? [] : [0, 1, 2, 3, 4, 5, 6] })
}

function addRule(): void {
  if (props.modelValue.rules.length >= kMaxScheduleRules) {
    return
  }
  emit('update:modelValue', { ...props.modelValue, rules: [...props.modelValue.rules, defaultScheduleRule()] })
}

function removeRule(index: number): void {
  emit('update:modelValue', {
    ...props.modelValue,
    rules: props.modelValue.rules.filter((_, ruleIndex) => ruleIndex !== index),
  })
}
</script>
