<template>
  <v-sheet
    :border="!compact"
    :rounded="!compact"
    :class="compact ? 'pa-0 flex-grow-1' : 'pa-3'"
    width="100%"
  >
    <analog-schedule-grid-controls
      v-if="editable && !compact"
      v-model:time-step="timeGridStepInput"
      v-model:level-step="levelGridStepInput"
      v-model:enabled="gridEnabled"
      @normalize="normalizeGridInputs"
    />
    <svg
      ref="chartRef"
      viewBox="0 0 1000 320"
      width="100%"
      :height="compact ? 104 : 320"
      role="img"
      :aria-label="t('device.fields.schedulePreview')"
      @pointermove="onPointerMove"
      @pointerleave="onPointerLeave"
      @contextmenu="openChartContextMenu"
    >
      <defs>
        <linearGradient :id="daylightGradientId" x1="0%" y1="0%" x2="100%" y2="0%">
          <stop
            v-for="(stop, index) in daylightGradientStops"
            :key="`${stop.offset}-${index}`"
            :offset="stop.offset"
            :stop-color="stop.color"
          />
        </linearGradient>
      </defs>
      <rect
        :x="plotLeft"
        :y="plotTop"
        :width="plotRight - plotLeft"
        :height="plotBottom - plotTop"
        :fill="`url(#${daylightGradientId})`"
        fill-opacity="0.72"
        pointer-events="none"
      />
      <line
        v-for="minute in minorMinuteGridTicks"
        :key="`xm-${minute}`"
        :x1="x(minute)"
        :x2="x(minute)"
        :y1="plotTop"
        :y2="plotBottom"
        stroke="rgb(var(--v-theme-outline-variant))"
        stroke-opacity="0.45"
        pointer-events="none"
      />
      <line
        v-for="level in minorLevelGridTicks"
        :key="`ym-${level}`"
        :x1="plotLeft"
        :x2="plotRight"
        :y1="y(level)"
        :y2="y(level)"
        stroke="rgb(var(--v-theme-outline-variant))"
        stroke-opacity="0.45"
        pointer-events="none"
      />
      <template v-if="showChartGrid">
        <line
          v-for="tick in yTicks"
          :key="`y-${tick}`"
          :x1="plotLeft"
          :x2="plotRight"
          :y1="y(tick)"
          :y2="y(tick)"
          stroke="rgb(var(--v-theme-outline-variant))"
          pointer-events="none"
        />
        <line
          v-for="hour in xTicks"
          :key="`x-${hour}`"
          :x1="x(hour * 60)"
          :x2="x(hour * 60)"
          :y1="plotTop"
          :y2="plotBottom"
          stroke="rgb(var(--v-theme-outline-variant))"
          pointer-events="none"
        />
      </template>
      <line
        :x1="x(daylightWindow.sunriseMinute)"
        :x2="x(daylightWindow.sunriseMinute)"
        :y1="plotTop"
        :y2="plotBottom"
        stroke="rgb(var(--v-theme-warning))"
        stroke-width="2"
        stroke-dasharray="4 4"
        vector-effect="non-scaling-stroke"
        pointer-events="none"
      />
      <line
        :x1="x(daylightWindow.sunsetMinute)"
        :x2="x(daylightWindow.sunsetMinute)"
        :y1="plotTop"
        :y2="plotBottom"
        stroke="rgb(var(--v-theme-info))"
        stroke-width="2"
        stroke-dasharray="4 4"
        vector-effect="non-scaling-stroke"
        pointer-events="none"
      />
      <template v-if="!compact">
        <text
          v-for="tick in yTicks"
          :key="`yl-${tick}`"
          :x="plotLeft - 10"
          :y="y(tick) + 5"
          text-anchor="end"
          :fill="axisTextColor"
          pointer-events="none"
        >
          {{ tick }}%
        </text>
        <text
          v-for="hour in xTicks"
          :key="`xl-${hour}`"
          :x="x(hour * 60)"
          y="305"
          text-anchor="middle"
          :fill="axisTextColor"
          pointer-events="none"
        >
          {{ String(hour).padStart(2, '0') }}:00
        </text>
      </template>
      <polyline
        v-for="(channel, index) in channels"
        :key="channel.id"
        :points="polyline(channel.points)"
        fill="none"
        :stroke="channelColor(index).svg"
        :stroke-width="selectedChannelId === channel.id ? 5 : 3"
        vector-effect="non-scaling-stroke"
        pointer-events="none"
      />
      <template v-if="editable && selectedChannel">
        <g
          v-for="point in selectedPointHandles"
          :key="`${selectedChannel.id}-${point.index}`"
          role="slider"
          tabindex="0"
          cursor="grab"
          :aria-label="`${selectedChannel.name}, ${formatAnalogScheduleTime(point.value.minuteOfDay)}, ${point.value.state}%`"
          :aria-valuetext="`${formatAnalogScheduleTime(point.value.minuteOfDay)}, ${point.value.state}%`"
          @pointerdown.stop.prevent="startDrag($event, selectedChannel.id, point.index)"
          @contextmenu.stop.prevent="openPointContextMenu($event, selectedChannel.id, point.index)"
          @keydown="movePointWithKeyboard($event, selectedChannel.id, point.index)"
        >
          <circle
            :cx="x(point.value.minuteOfDay)"
            :cy="y(point.value.state)"
            r="36"
            fill="transparent"
          />
          <circle
            :cx="x(point.value.minuteOfDay)"
            :cy="y(point.value.state)"
            r="14"
            :fill="selectedChannelColor"
            stroke="rgb(var(--v-theme-on-surface-variant))"
            stroke-width="4"
            vector-effect="non-scaling-stroke"
            pointer-events="none"
          />
        </g>
      </template>
      <line
        :x1="x(nowMinute)"
        :x2="x(nowMinute)"
        :y1="plotTop"
        :y2="plotBottom"
        stroke="rgb(var(--v-theme-error))"
        stroke-width="2"
        stroke-dasharray="6 6"
        vector-effect="non-scaling-stroke"
        pointer-events="none"
      />
      <line
        v-if="!compact && hoverMinute !== null"
        :x1="x(hoverMinute)"
        :x2="x(hoverMinute)"
        :y1="plotTop"
        :y2="plotBottom"
        stroke="rgb(var(--v-theme-on-surface))"
        vector-effect="non-scaling-stroke"
        pointer-events="none"
      />
      <g
        v-if="dragTooltip"
        :transform="`translate(${dragTooltip.x} ${dragTooltip.y})`"
        pointer-events="none"
      >
        <rect
          :width="dragTooltipWidth"
          :height="dragTooltipHeight"
          rx="8"
          fill="rgb(var(--v-theme-surface))"
          stroke="rgb(var(--v-theme-outline))"
          stroke-width="2"
          vector-effect="non-scaling-stroke"
        />
        <text
          :x="dragTooltipWidth / 2"
          y="23"
          text-anchor="middle"
          :fill="axisTextColor"
        >
          {{ dragTooltip.text }}
        </text>
      </g>
    </svg>

    <v-menu
      v-model="contextMenu.open"
      :target="contextMenu.target"
      location="bottom start"
    >
      <v-list density="compact">
        <template v-if="contextMenu.pointIndex !== null">
          <v-list-item
            :title="t('device.actions.insertSchedulePointBefore')"
            :disabled="!contextCanInsert"
            @click="insertContextPoint(-1)"
          />
          <v-list-item
            :title="t('device.actions.insertSchedulePointAfter')"
            :disabled="!contextCanInsert"
            @click="insertContextPoint(1)"
          />
          <v-divider />
          <v-list-item
            :title="t('device.actions.delete')"
            :disabled="!contextCanDelete"
            @click="deleteContextPoint"
          />
        </template>
        <v-list-item
          v-else
          :title="t('device.actions.addSchedulePoint')"
          :disabled="!contextCanInsert"
          @click="addContextPoint"
        />
      </v-list>
    </v-menu>

    <template v-if="!compact">
      <div class="d-flex flex-wrap ga-2">
        <v-chip
          v-for="(channel, index) in channels"
          :key="channel.id"
          size="small"
          :variant="selectedChannelId === channel.id ? 'tonal' : 'outlined'"
          color="on-surface-variant"
          :disabled="editable && channel.editable === false"
          @click="selectChannel(channel)"
        >
          <v-icon start icon="analog-output" :color="channelColor(index).theme" />
          {{ channel.name }}: {{ sampleAnalogSchedule(channel.points, hoverMinute ?? nowMinute) }}%
        </v-chip>
      </div>
      <analog-schedule-daylight-legend
        :daylight="daylightWindow"
        :has-coordinates="daylightStore.coordinates !== null"
        :location-status="daylightStore.locationStatus"
        @request-location="daylightStore.requestBrowserLocation()"
      />
      <div v-if="editable && editableChannels.length > 0" class="text-body-small text-medium-emphasis mt-2">
        {{ t('device.fields.scheduleDragHint') }}
      </div>
    </template>
  </v-sheet>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, useId, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { useTheme } from 'vuetify'

import AnalogScheduleDaylightLegend from '@/components/devices/analog-output/AnalogScheduleDaylightLegend.vue'
import AnalogScheduleGridControls from '@/components/devices/analog-output/AnalogScheduleGridControls.vue'
import { calculateAnalogDaylightWindow } from '@/models/devices/analog-daylight'
import {
  addAnalogSchedulePoint,
  activeAnalogSchedulePoints,
  analogScheduleDefaultLevelGridStepPercent,
  analogScheduleDefaultTimeGridStepMinutes,
  analogScheduleMaximumPointCount,
  analogScheduleMaximumTimeGridStepMinutes,
  analogScheduleMinimumTimeGridStepMinutes,
  analogScheduleMinutesPerDay,
  deleteAnalogSchedulePoint,
  formatAnalogScheduleTime,
  insertAnalogSchedulePoint,
  moveAnalogSchedulePoint,
  normalizeAnalogScheduleGridStep,
  sampleAnalogSchedule,
  type AnalogScheduleChartChannel,
} from '@/models/devices/analog-schedule'
import { useDaylightStore } from '@/stores/daylight'

const props = withDefaults(defineProps<{
  channels: AnalogScheduleChartChannel[]
  compact?: boolean
  editable?: boolean
}>(), {
  compact: false,
  editable: false,
})

const emit = defineEmits<{
  'update:channel-points': [channelId: number, points: AnalogScheduleChartChannel['points']]
  'edit-end': [channelId: number]
}>()

const { t } = useI18n()
const theme = useTheme()
const daylightStore = useDaylightStore()
const chartRef = ref<SVGSVGElement | null>(null)
const hoverMinute = ref<number | null>(null)
const nowMinute = ref(0)
const currentDate = ref(new Date())
const selectedChannelId = ref<number | null>(null)
const gridEnabled = ref(true)
const timeGridStepInput = ref(String(analogScheduleDefaultTimeGridStepMinutes))
const levelGridStepInput = ref(String(analogScheduleDefaultLevelGridStepPercent))
const drag = ref<{
  channelId: number
  pointIndex: number
  pointerId: number
  pointerX: number
  pointerY: number
  minuteOfDay: number
  state: number
} | null>(null)
const contextMenu = ref<{
  open: boolean
  target: [number, number]
  channelId: number | null
  pointIndex: number | null
  minuteOfDay: number
  state: number
}>({
  open: false,
  target: [0, 0],
  channelId: null,
  pointIndex: null,
  minuteOfDay: 0,
  state: 0,
})
let clockTimer: ReturnType<typeof window.setInterval> | null = null

const palette = [
  { theme: 'primary', svg: 'rgb(var(--v-theme-primary))' },
  { theme: 'secondary', svg: 'rgb(var(--v-theme-secondary))' },
  { theme: 'info', svg: 'rgb(var(--v-theme-info))' },
  { theme: 'success', svg: 'rgb(var(--v-theme-success))' },
  { theme: 'warning', svg: 'rgb(var(--v-theme-warning))' },
  { theme: 'error', svg: 'rgb(var(--v-theme-error))' },
]
const yTicks = [0, 25, 50, 75, 100]
const xTicks = [0, 4, 8, 12, 16, 20, 24]
const plotLeft = computed(() => props.compact ? 20 : 60)
const plotRight = computed(() => props.compact ? 980 : 980)
const plotTop = 20
const plotBottom = 280
const dragTooltipWidth = 150
const dragTooltipHeight = 36
const axisTextColor = computed(() => String(theme.current.value.colors.onSurfaceVariant))
const daylightGradientId = `${useId().replaceAll(':', '-')}-daylight`
const daylightWindow = computed(() =>
  calculateAnalogDaylightWindow(currentDate.value, daylightStore.coordinates),
)
const daylightGradientStops = computed(() => {
  const colors = theme.current.value.colors
  const sunrise = daylightWindow.value.sunriseMinute
  const sunset = daylightWindow.value.sunsetMinute
  const offset = (minute: number): string =>
    `${Math.min(100, Math.max(0, minute / analogScheduleMinutesPerDay * 100))}%`
  return [
    { offset: '0%', color: String(colors.infoContainer) },
    { offset: offset(sunrise - 60), color: String(colors.infoContainer) },
    { offset: offset(sunrise + 45), color: String(colors.warningContainer) },
    { offset: offset(sunset - 45), color: String(colors.warningContainer) },
    { offset: offset(sunset + 60), color: String(colors.infoContainer) },
    { offset: '100%', color: String(colors.infoContainer) },
  ]
})
const timeGridStep = computed(() =>
  normalizeAnalogScheduleGridStep(
    Number(timeGridStepInput.value),
    analogScheduleMinimumTimeGridStepMinutes,
    analogScheduleMaximumTimeGridStepMinutes,
    analogScheduleDefaultTimeGridStepMinutes,
  ),
)
const levelGridStep = computed(() =>
  normalizeAnalogScheduleGridStep(
    Number(levelGridStepInput.value),
    1,
    100,
    analogScheduleDefaultLevelGridStepPercent,
  ),
)
const effectiveTimeStep = computed(() => gridEnabled.value ? timeGridStep.value : 1)
const effectiveLevelStep = computed(() => gridEnabled.value ? levelGridStep.value : 1)
const showChartGrid = computed(() => !props.editable || gridEnabled.value)
const minorMinuteGridTicks = computed(() => {
  if (!props.editable || !gridEnabled.value) {
    return []
  }
  const majorTicks = new Set(xTicks.map(hour => hour * 60))
  return buildGridTicks(analogScheduleMinutesPerDay, timeGridStep.value)
    .filter(minute => !majorTicks.has(minute))
})
const minorLevelGridTicks = computed(() => {
  if (!props.editable || !gridEnabled.value) {
    return []
  }
  const majorTicks = new Set(yTicks)
  return buildGridTicks(100, levelGridStep.value)
    .filter(level => !majorTicks.has(level))
})
const dragTooltip = computed(() => {
  const activeDrag = drag.value
  if (activeDrag === null) {
    return null
  }
  const preferredX = activeDrag.pointerX + 18
  const tooltipX = preferredX + dragTooltipWidth <= plotRight.value
    ? preferredX
    : activeDrag.pointerX - dragTooltipWidth - 18
  const preferredY = activeDrag.pointerY - dragTooltipHeight - 18
  const tooltipY = preferredY >= plotTop
    ? preferredY
    : activeDrag.pointerY + 18
  return {
    x: Math.min(
      plotRight.value - dragTooltipWidth,
      Math.max(plotLeft.value, tooltipX),
    ),
    y: Math.min(
      plotBottom - dragTooltipHeight,
      Math.max(plotTop, tooltipY),
    ),
    text: `${formatAnalogScheduleTime(activeDrag.minuteOfDay)} · ${activeDrag.state}%`,
  }
})

const editableChannels = computed(() =>
  props.channels.filter(channel => channel.editable !== false),
)
const selectedChannel = computed(() =>
  props.channels.find(channel => channel.id === selectedChannelId.value) ?? null,
)
const selectedChannelColor = computed(() => {
  const index = props.channels.findIndex(channel => channel.id === selectedChannelId.value)
  return channelColor(Math.max(0, index)).svg
})
const selectedPointHandles = computed(() =>
  (selectedChannel.value?.points ?? [])
    .map((value, index) => ({ value, index }))
    .filter(point => !point.value.deleted),
)
const contextChannel = computed(() =>
  props.channels.find(channel => channel.id === contextMenu.value.channelId) ?? null,
)
const contextCanInsert = computed(() =>
  contextChannel.value !== null
  && contextChannel.value.points.filter(point => !point.deleted).length < analogScheduleMaximumPointCount,
)
const contextCanDelete = computed(() =>
  contextChannel.value !== null
  && contextChannel.value.points.filter(point => !point.deleted).length > 1,
)

watch(
  editableChannels,
  channels => {
    if (!props.editable || channels.length === 0) {
      selectedChannelId.value = null
      return
    }
    if (!channels.some(channel => channel.id === selectedChannelId.value)) {
      selectedChannelId.value = channels[0].id
    }
  },
  { immediate: true },
)

function updateNowMinute(): void {
  const now = new Date()
  currentDate.value = now
  nowMinute.value = now.getHours() * 60 + now.getMinutes()
}

onMounted(() => {
  updateNowMinute()
  clockTimer = window.setInterval(updateNowMinute, 60000)
})

onBeforeUnmount(() => {
  if (clockTimer !== null) {
    window.clearInterval(clockTimer)
  }
  removeDragListeners()
})

function channelColor(index: number): (typeof palette)[number] {
  return palette[index % palette.length]
}

function buildGridTicks(maximum: number, step: number): number[] {
  const ticks: number[] = []
  for (let value = 0; value <= maximum; value += step) {
    ticks.push(value)
  }
  if (ticks[ticks.length - 1] !== maximum) {
    ticks.push(maximum)
  }
  return ticks
}

function normalizeGridInputs(): void {
  timeGridStepInput.value = String(timeGridStep.value)
  levelGridStepInput.value = String(levelGridStep.value)
}

function x(minute: number): number {
  const bounded = Math.min(analogScheduleMinutesPerDay, Math.max(0, minute))
  return plotLeft.value + (bounded / analogScheduleMinutesPerDay) * (plotRight.value - plotLeft.value)
}

function y(percent: number): number {
  const bounded = Math.min(100, Math.max(0, percent))
  return plotBottom - (bounded / 100) * (plotBottom - plotTop)
}

function polyline(points: AnalogScheduleChartChannel['points']): string {
  const activePoints = activeAnalogSchedulePoints(points)
  const path = [{ minuteOfDay: 0, state: sampleAnalogSchedule(points, 0) }]
  path.push(...activePoints.filter(point => point.minuteOfDay > 0))
  path.push({ minuteOfDay: analogScheduleMinutesPerDay, state: sampleAnalogSchedule(points, 0) })
  return path.map(point => `${x(point.minuteOfDay)},${y(point.state)}`).join(' ')
}

function pointerPosition(event: Pick<MouseEvent, 'clientX' | 'clientY'>): DOMPoint | null {
  const matrix = chartRef.value?.getScreenCTM()
  if (matrix === undefined || matrix === null) {
    return null
  }
  return new DOMPoint(event.clientX, event.clientY).matrixTransform(matrix.inverse())
}

function minuteAt(point: DOMPoint): number {
  return Math.round(
    Math.min(1, Math.max(0, (point.x - plotLeft.value) / (plotRight.value - plotLeft.value)))
      * (analogScheduleMinutesPerDay - 1),
  )
}

function stateAt(point: DOMPoint): number {
  return Math.round(
    Math.min(1, Math.max(0, (plotBottom - point.y) / (plotBottom - plotTop))) * 100,
  )
}

function selectChannel(channel: AnalogScheduleChartChannel): void {
  if (props.editable && channel.editable !== false) {
    selectedChannelId.value = channel.id
  }
}

function startDrag(event: PointerEvent, channelId: number, pointIndex: number): void {
  if (!props.editable || event.button !== 0) {
    return
  }
  const channel = props.channels.find(entry => entry.id === channelId)
  const point = channel?.points[pointIndex]
  const pointer = pointerPosition(event)
  if (point === undefined || pointer === null) {
    return
  }
  removeDragListeners()
  selectedChannelId.value = channelId
  drag.value = {
    channelId,
    pointIndex,
    pointerId: event.pointerId,
    pointerX: pointer.x,
    pointerY: pointer.y,
    minuteOfDay: point.minuteOfDay,
    state: point.state,
  }
  window.addEventListener('pointermove', onDragPointerMove, { passive: false })
  window.addEventListener('pointerup', finishDrag)
  window.addEventListener('pointercancel', finishDrag)
}

function openContextMenu(
  event: MouseEvent,
  channelId: number,
  pointIndex: number | null,
  minuteOfDay: number,
  state: number,
): void {
  contextMenu.value = {
    open: true,
    target: [event.clientX, event.clientY],
    channelId,
    pointIndex,
    minuteOfDay,
    state,
  }
}

function openPointContextMenu(event: MouseEvent, channelId: number, pointIndex: number): void {
  if (!props.editable) {
    return
  }
  const channel = props.channels.find(entry => entry.id === channelId)
  const point = channel?.points[pointIndex]
  if (channel === undefined || channel.editable === false || point === undefined || point.deleted) {
    return
  }
  selectedChannelId.value = channelId
  openContextMenu(event, channelId, pointIndex, point.minuteOfDay, point.state)
}

function openChartContextMenu(event: MouseEvent): void {
  const channel = selectedChannel.value
  if (!props.editable || channel === null || channel.editable === false) {
    return
  }
  const pointer = pointerPosition(event)
  if (
    pointer === null
    || pointer.x < plotLeft.value
    || pointer.x > plotRight.value
    || pointer.y < plotTop
    || pointer.y > plotBottom
  ) {
    return
  }
  event.preventDefault()
  openContextMenu(event, channel.id, null, minuteAt(pointer), stateAt(pointer))
}

function emitContextPoints(points: AnalogScheduleChartChannel['points']): void {
  const channelId = contextMenu.value.channelId
  if (channelId === null) {
    return
  }
  emit('update:channel-points', channelId, points)
  emit('edit-end', channelId)
  contextMenu.value.open = false
}

function addContextPoint(): void {
  const channel = contextChannel.value
  if (channel === null) {
    return
  }
  emitContextPoints(addAnalogSchedulePoint(
    channel.points,
    contextMenu.value.minuteOfDay,
    contextMenu.value.state,
    effectiveTimeStep.value,
    effectiveLevelStep.value,
  ))
}

function deleteContextPoint(): void {
  const channel = contextChannel.value
  const pointIndex = contextMenu.value.pointIndex
  if (channel === null || pointIndex === null) {
    return
  }
  emitContextPoints(deleteAnalogSchedulePoint(channel.points, pointIndex))
}

function insertContextPoint(direction: -1 | 1): void {
  const channel = contextChannel.value
  const pointIndex = contextMenu.value.pointIndex
  if (channel === null || pointIndex === null) {
    return
  }
  emitContextPoints(insertAnalogSchedulePoint(channel.points, pointIndex, direction))
}

function updatePoint(
  channelId: number,
  pointIndex: number,
  minuteOfDay: number,
  state: number,
): AnalogScheduleChartChannel['points'][number] | null {
  const channel = props.channels.find(entry => entry.id === channelId)
  if (channel === undefined || channel.editable === false) {
    return null
  }
  const points = moveAnalogSchedulePoint(
    channel.points,
    pointIndex,
    minuteOfDay,
    state,
    effectiveTimeStep.value,
    effectiveLevelStep.value,
  )
  emit(
    'update:channel-points',
    channelId,
    points,
  )
  return points[pointIndex] ?? null
}

function onPointerMove(event: PointerEvent): void {
  if (drag.value !== null) {
    return
  }
  const point = pointerPosition(event)
  if (point === null) {
    return
  }
  hoverMinute.value = minuteAt(point)
}

function onDragPointerMove(event: PointerEvent): void {
  if (drag.value === null || drag.value.pointerId !== event.pointerId) {
    return
  }
  const point = pointerPosition(event)
  if (point === null) {
    return
  }
  event.preventDefault()
  const movedPoint = updatePoint(
    drag.value.channelId,
    drag.value.pointIndex,
    minuteAt(point),
    stateAt(point),
  )
  if (movedPoint === null) {
    return
  }
  hoverMinute.value = movedPoint.minuteOfDay
  drag.value = {
    ...drag.value,
    pointerX: point.x,
    pointerY: point.y,
    minuteOfDay: movedPoint.minuteOfDay,
    state: movedPoint.state,
  }
}

function removeDragListeners(): void {
  window.removeEventListener('pointermove', onDragPointerMove)
  window.removeEventListener('pointerup', finishDrag)
  window.removeEventListener('pointercancel', finishDrag)
}

function finishDrag(event: PointerEvent): void {
  if (drag.value === null || drag.value.pointerId !== event.pointerId) {
    return
  }
  const channelId = drag.value.channelId
  drag.value = null
  removeDragListeners()
  emit('edit-end', channelId)
}

function onPointerLeave(): void {
  if (drag.value === null) {
    hoverMinute.value = null
  }
}

function movePointWithKeyboard(
  event: KeyboardEvent,
  channelId: number,
  pointIndex: number,
): void {
  const channel = props.channels.find(entry => entry.id === channelId)
  const point = channel?.points[pointIndex]
  if (channel === undefined || point === undefined) {
    return
  }

  let minute = point.minuteOfDay
  let state = point.state
  if (event.key === 'ArrowLeft') {
    minute -= effectiveTimeStep.value
  } else if (event.key === 'ArrowRight') {
    minute += effectiveTimeStep.value
  } else if (event.key === 'ArrowDown') {
    state -= effectiveLevelStep.value
  } else if (event.key === 'ArrowUp') {
    state += effectiveLevelStep.value
  } else {
    return
  }
  event.preventDefault()
  updatePoint(channelId, pointIndex, minute, state)
  emit('edit-end', channelId)
}
</script>
