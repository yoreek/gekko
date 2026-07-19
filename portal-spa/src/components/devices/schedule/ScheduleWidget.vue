<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="time" />
    </template>
    <v-chip :color="statusPreview.active ? 'success' : 'secondary'" variant="tonal" size="small">
      {{ statusPreview.active ? t('device.dialog.schedule.currentlyOn') : t('device.dialog.schedule.currentlyOff') }}
    </v-chip>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column align-center ga-2 pa-2">
    <v-chip :color="statusPreview.active ? 'success' : 'secondary'" variant="tonal">
      {{ statusPreview.active ? t('device.dialog.schedule.currentlyOn') : t('device.dialog.schedule.currentlyOff') }}
    </v-chip>
    <div v-if="nextChangeText" class="text-body-small text-medium-emphasis">{{ nextChangeText }}</div>
    <div class="text-body-small text-medium-emphasis">{{ t('device.dialog.schedule.estimateNote') }}</div>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord, ScheduleRuleConfig } from '@/api/contracts'
import { describeScheduleStatus } from '@/models/devices/schedule-preview'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

const props = withDefaults(
  defineProps<{
    device: DeviceRecord
    editable?: boolean
    dense?: boolean
  }>(),
  {
    dense: true,
  },
)

const { t } = useI18n()

// The firmware never pushes a live "active" value (ScheduleDevice never marks itself
// runtime-dirty) - computed client-side from the rule config against the browser's own clock
// instead, see schedule-preview.ts. Refreshed once a minute.
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

const rules = computed(() => {
  const config = props.device.config as unknown as Record<string, unknown>
  return Array.isArray(config.rules) ? (config.rules as ScheduleRuleConfig[]) : []
})

const statusPreview = computed(() => describeScheduleStatus(rules.value, now.value))

function formatNextChange(at: Date): string {
  const timeText = `${String(at.getHours()).padStart(2, '0')}:${String(at.getMinutes()).padStart(2, '0')}`
  const sameDay = at.getFullYear() === now.value.getFullYear() && at.getMonth() === now.value.getMonth() && at.getDate() === now.value.getDate()
  if (sameDay) {
    return timeText
  }
  return `${t(`device.dialog.schedule.weekdayShort.${at.getDay()}`)} ${timeText}`
}

const nextChangeText = computed(() => {
  const { active, nextChangeAt } = statusPreview.value
  if (!nextChangeAt) {
    return ''
  }
  return t(active ? 'device.dialog.schedule.turnsOffAt' : 'device.dialog.schedule.turnsOnAt', { time: formatNextChange(nextChangeAt) })
})
</script>
