<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.time')" :subtitle="t('time.subtitle')">
          <template #actions>
            <v-progress-circular
              v-if="timeLifecycle.initialLoading.value"
              indeterminate
              color="primary"
              size="24"
              width="2"
            />
            <v-chip
              v-else-if="timeLifecycle.ready.value && timeStore.synced"
              variant="tonal"
              :color="sourceChipColor"
              size="small"
              class="mr-2"
            >
              {{ sourceLabel }}
            </v-chip>
            <v-chip
              v-if="timeLifecycle.ready.value"
              variant="tonal"
              :color="statusChipColor"
              size="small"
            >
              {{ statusChipLabel }}
            </v-chip>
          </template>
        </PageToolbar>
      </template>

      <v-progress-linear
        v-if="timeLifecycle.initialLoading.value || timeLifecycle.refreshing.value"
        indeterminate
        color="primary"
      />

      <v-alert
        v-else-if="timeLifecycle.loadError.value !== null && !timeLifecycle.ready.value"
        type="error"
        variant="tonal"
      >
        {{ loadErrorMessage }}
      </v-alert>

      <v-row v-else-if="timeLifecycle.ready.value" class="ga-4">
        <v-col cols="12" sm="3">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('time.syncStatus') }}</div>
            <div class="text-title-large">{{ statusChipLabel }}</div>
          </div>
        </v-col>
        <v-col cols="12" sm="3">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('time.sourceLabel') }}</div>
            <div class="text-title-large">{{ timeStore.synced ? sourceLabel : '—' }}</div>
          </div>
        </v-col>
        <v-col cols="12" sm="3">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('time.currentTime') }}</div>
            <div class="text-title-large">{{ currentTimeLabel }}</div>
          </div>
        </v-col>
        <v-col cols="12" sm="3">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('time.lastSync') }}</div>
            <div class="text-title-large">{{ lastSyncLabel }}</div>
          </div>
        </v-col>
      </v-row>

      <template #actions>
        <v-btn
          variant="outlined"
          :loading="syncingNow"
          :disabled="!timeLifecycle.ready.value || timeLifecycle.busy.value"
          size="small"
          class="mr-2"
          @click="syncNow"
        >
          {{ t('time.syncNow') }}
        </v-btn>
        <v-btn
          :loading="timeLifecycle.refreshing.value"
          :disabled="timeLifecycle.busy.value || timeLifecycle.dirty.value"
          color="primary"
          size="small"
          @click="refreshTimeStatus"
        >
          {{ t('actions.refresh') }}
        </v-btn>
      </template>
    </PageCard>

    <PageCard class="mt-4">
      <template #header>
        <PageToolbar :title="t('time.settingsTitle')" />
      </template>

      <v-progress-linear
        v-if="timeLifecycle.initialLoading.value || timeLifecycle.refreshing.value"
        indeterminate
        color="primary"
      />

      <v-alert
        v-else-if="timeLifecycle.loadError.value !== null && !timeLifecycle.ready.value"
        type="error"
        variant="tonal"
      >
        {{ loadErrorMessage }}
      </v-alert>

      <v-form v-else-if="timeDraft" :disabled="timeLifecycle.busy.value">
        <div class="d-flex flex-column ga-4">
          <v-switch v-model="timeDraft.enabled" :label="t('time.enabled')" inset hide-details />
          <v-text-field
            v-select-on-focus
            v-model="timeDraft.ntpServer"
            :label="t('time.ntpServer')"
            :disabled="!timeDraft.enabled"
            autocomplete="off"
          />
          <v-select
            v-model="timeDraft.timezoneId"
            :label="t('time.timezone')"
            :items="timezones"
            item-title="name"
            item-value="id"
          />
          <v-text-field
            v-select-on-focus
            v-model.number="timeDraft.syncIntervalSeconds"
            type="number"
            :label="t('time.syncInterval')"
            :disabled="!timeDraft.enabled"
            :hint="t('time.syncIntervalHint')"
            persistent-hint
          />

          <v-alert v-if="timeLifecycle.loadError.value !== null" type="error" variant="tonal">
            {{ loadErrorMessage }}
          </v-alert>
          <v-alert v-if="timeLifecycle.saveError.value !== null" type="error" variant="tonal">
            {{ saveErrorMessage }}
          </v-alert>
        </div>
      </v-form>

      <template #actions>
        <v-btn
          :loading="timeLifecycle.saving.value"
          :disabled="!timeLifecycle.canSave.value"
          color="primary"
          size="small"
          @click="saveSettings"
        >
          {{ t('actions.save') }}
        </v-btn>
      </template>
    </PageCard>

    <PageCard class="mt-4">
      <template #header>
        <PageToolbar :title="t('time.manualSetTitle')" :subtitle="t('time.manualSetSubtitle')" />
      </template>

      <v-form :disabled="!timeLifecycle.ready.value || timeLifecycle.busy.value || manualSetting">
        <div class="d-flex flex-column ga-4">
          <v-text-field v-model="manualDateTime" type="datetime-local" :label="t('time.manualSetLabel')" />

          <v-alert v-if="manualSetError" type="error" variant="tonal">
            {{ manualSetError }}
          </v-alert>
        </div>
      </v-form>

      <template #actions>
        <v-btn
          :loading="manualSetting"
          :disabled="!timeLifecycle.ready.value || timeLifecycle.busy.value || !manualDateTime"
          color="primary"
          size="small"
          @click="setManualTime"
        >
          {{ t('time.manualSetApply') }}
        </v-btn>
      </template>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchTimeSettings, fetchTimeStatus, fetchTimezones, setSystemTime, updateTimeSettings } from '@/api'
import { useAsyncForm } from '@/composables/useAsyncForm'
import {
  buildTimeSettingsPayload,
  createTimeSettingsDraft,
  isTimeSettingsDirty,
  type TimeSettingsDraft,
  type TimeSettingsSnapshot,
} from '@/models/time-settings-form'
import { useTimeStore } from '@/stores/time'
import { useNotificationsStore } from '@/stores/notifications'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const timeStore = useTimeStore()
const notifications = useNotificationsStore()

const syncingNow = ref(false)
const manualSetting = ref(false)
const manualSetError = ref('')
const manualDateTime = ref('')

const timeLifecycle = useAsyncForm<TimeSettingsSnapshot, TimeSettingsDraft>({
  load: loadTimeSettingsSnapshot,
  createDraft: createTimeSettingsDraft,
  isDirty: isTimeSettingsDirty,
  save: saveTimeSettingsSnapshot,
  onCommit: commitTimeSettingsSnapshot,
})

const timeDraft = computed(() => timeLifecycle.draft.value)
const timezones = computed(() => timeLifecycle.source.value?.timezones ?? [])
const loadErrorMessage = computed(() => formatError(timeLifecycle.loadError.value, t('notifications.error')))
const saveErrorMessage = computed(() => formatError(timeLifecycle.saveError.value, t('notifications.error')))

const statusChipLabel = computed(() => {
  if (!timeStore.enabled) return t('time.status.disabled')
  if (timeStore.synced) return t('time.status.synced')
  if (timeStore.waitingForStation) return t('time.status.waitingForStation')
  return t('time.status.notSynced')
})

const statusChipColor = computed(() => {
  if (!timeStore.enabled) return 'secondary'
  if (timeStore.synced) return 'success'
  if (timeStore.waitingForStation) return 'warning'
  return 'secondary'
})

const sourceLabel = computed(() => t(`time.source.${timeStore.source}`))
const sourceChipColor = computed(() => (timeStore.source === 'rtc' ? 'warning' : 'secondary'))

// "2024-03-05T12:45:55+02:00"/"...Z" -> "2024-03-05 12:45:55" - drops the sub-second/offset
// noise the raw ISO 8601 string carries, since the offset is already implied by "local".
function toHumanDateTime(iso8601: string): string {
  return iso8601.slice(0, 19).replace('T', ' ')
}

// Renders a UTC epoch as a human "YYYY-MM-DD HH:MM:SS" string in the given UTC offset, without
// pulling in a timezone library: shifting the epoch before formatting as UTC prints the shifted
// (i.e. local) wall-clock components.
function formatEpochAsLocalHuman(epochUtcSeconds: number, offsetMinutes: number): string {
  const localMs = (epochUtcSeconds + offsetMinutes * 60) * 1000
  return toHumanDateTime(new Date(localMs).toISOString())
}

const currentTimeLabel = computed(() => (timeStore.synced && timeStore.localTimeIso8601 ? toHumanDateTime(timeStore.localTimeIso8601) : '—'))
const lastSyncLabel = computed(() => (timeStore.synced && timeStore.lastSyncEpochUtc > 0
  ? formatEpochAsLocalHuman(timeStore.lastSyncEpochUtc, timeStore.utcOffsetMinutes)
  : '—'))

function formatUtcOffset(offsetMinutes: number): string {
  const sign = offsetMinutes < 0 ? '-' : '+'
  const absMinutes = Math.abs(offsetMinutes)
  const hours = Math.floor(absMinutes / 60)
  const minutes = absMinutes % 60
  return `${sign}${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}`
}

function formatError(error: unknown, fallback: string): string {
  return error instanceof Error && error.message.length > 0 ? error.message : fallback
}

async function loadTimeSettingsSnapshot(): Promise<TimeSettingsSnapshot> {
  const [status, settings, catalog] = await Promise.all([
    fetchTimeStatus(),
    fetchTimeSettings(),
    fetchTimezones(),
  ])
  return { status, settings, timezones: catalog.timezones }
}

async function saveTimeSettingsSnapshot(
  { source, draft }: { source: TimeSettingsSnapshot; draft: TimeSettingsDraft },
): Promise<TimeSettingsSnapshot> {
  const settings = await updateTimeSettings(buildTimeSettingsPayload(draft))
  const status = await fetchTimeStatus()
  return { status, settings, timezones: source.timezones }
}

function commitTimeSettingsSnapshot(source: TimeSettingsSnapshot): void {
  timeStore.replaceFromStatus(source.status)
  timeStore.replaceFromSettings(source.settings)
}

function refreshTimeStatus(): void {
  void timeLifecycle.refresh()
}

async function saveSettings(): Promise<void> {
  if (await timeLifecycle.save()) {
    notifications.notify(t('notifications.saved'), 'success')
  }
}

async function syncNow(): Promise<void> {
  // An unchanged settings PUT still forces the firmware to resync immediately instead of waiting
  // out the periodic interval - reuse it rather than adding a dedicated endpoint.
  syncingNow.value = true
  try {
    const settings = await updateTimeSettings({})
    timeStore.replaceFromSettings(settings)
    const status = await fetchTimeStatus()
    timeStore.replaceFromStatus(status)
    notifications.notify(t('time.syncNowSuccess'), 'success')
  } catch {
    notifications.notify(t('time.syncNowError'), 'error')
  } finally {
    syncingNow.value = false
  }
}

async function setManualTime(): Promise<void> {
  manualSetError.value = ''
  if (!manualDateTime.value) {
    manualSetError.value = t('time.manualSetRequired')
    return
  }
  manualSetting.value = true
  try {
    // <input type="datetime-local"> has no timezone of its own - treat the entered value as wall
    // clock time in the currently selected timezone, using the last-known UTC offset to build a
    // fully qualified ISO 8601 string the firmware can convert without any client-side DST logic.
    const iso8601 = `${manualDateTime.value}:00${formatUtcOffset(timeStore.utcOffsetMinutes)}`
    const status = await setSystemTime({ iso8601 })
    timeStore.replaceFromStatus(status)
    notifications.notify(t('time.manualSetSuccess'), 'success')
  } catch (error) {
    manualSetError.value = error instanceof Error && error.message.length > 0 ? error.message : t('time.manualSetError')
  } finally {
    manualSetting.value = false
  }
}

onMounted(() => {
  void timeLifecycle.initialize()
})
</script>
