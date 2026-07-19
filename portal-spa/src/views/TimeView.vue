<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.time')" :subtitle="t('time.subtitle')">
          <template #actions>
            <v-chip v-if="timeStore.synced" variant="tonal" :color="sourceChipColor" size="small" class="mr-2">
              {{ sourceLabel }}
            </v-chip>
            <v-chip variant="tonal" :color="statusChipColor" size="small">
              {{ statusChipLabel }}
            </v-chip>
          </template>
        </PageToolbar>
      </template>

      <v-row class="ga-4">
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
        <v-btn variant="outlined" :loading="syncingNow" size="small" class="mr-2" @click="syncNow">
          {{ t('time.syncNow') }}
        </v-btn>
        <v-btn :loading="loading" color="primary" size="small" @click="refreshTimeStatus">
          {{ t('actions.refresh') }}
        </v-btn>
      </template>
    </PageCard>

    <PageCard class="mt-4">
      <template #header>
        <PageToolbar :title="t('time.settingsTitle')" />
      </template>

      <div class="d-flex flex-column ga-4">
        <v-switch v-model="form.enabled" :label="t('time.enabled')" inset hide-details />
        <v-text-field
          v-select-on-focus
          v-model="form.ntpServer"
          :label="t('time.ntpServer')"
          :disabled="!form.enabled"
          autocomplete="off"
        />
        <v-select
          v-model="form.timezoneId"
          :label="t('time.timezone')"
          :items="timezones"
          item-title="name"
          item-value="id"
        />
        <v-text-field
          v-select-on-focus
          v-model.number="form.syncIntervalSeconds"
          type="number"
          :label="t('time.syncInterval')"
          :disabled="!form.enabled"
          :hint="t('time.syncIntervalHint')"
          persistent-hint
        />

        <v-alert v-if="errorMessage" type="error" variant="tonal">
          {{ errorMessage }}
        </v-alert>
      </div>

      <template #actions>
        <v-btn :loading="saving" color="primary" size="small" @click="saveSettings">
          {{ t('actions.save') }}
        </v-btn>
      </template>
    </PageCard>

    <PageCard class="mt-4">
      <template #header>
        <PageToolbar :title="t('time.manualSetTitle')" :subtitle="t('time.manualSetSubtitle')" />
      </template>

      <div class="d-flex flex-column ga-4">
        <v-text-field v-model="manualDateTime" type="datetime-local" :label="t('time.manualSetLabel')" />

        <v-alert v-if="manualSetError" type="error" variant="tonal">
          {{ manualSetError }}
        </v-alert>
      </div>

      <template #actions>
        <v-btn :loading="manualSetting" color="primary" size="small" @click="setManualTime">
          {{ t('time.manualSetApply') }}
        </v-btn>
      </template>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { onMounted, reactive, ref, computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchTimeSettings, fetchTimeStatus, fetchTimezones, setSystemTime, updateTimeSettings } from '@/api'
import type { TimezoneCatalogEntry } from '@/api'
import { useTimeStore } from '@/stores/time'
import { useNotificationsStore } from '@/stores/notifications'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const timeStore = useTimeStore()
const notifications = useNotificationsStore()

const loading = ref(false)
const saving = ref(false)
const syncingNow = ref(false)
const manualSetting = ref(false)
const errorMessage = ref('')
const manualSetError = ref('')
const timezones = ref<TimezoneCatalogEntry[]>([])
const manualDateTime = ref('')

const form = reactive({
  enabled: true,
  ntpServer: '',
  timezoneId: '',
  syncIntervalSeconds: 3600,
})

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

function applySettingsToForm(): void {
  form.enabled = timeStore.enabled
  form.ntpServer = timeStore.ntpServer
  form.timezoneId = timeStore.timezoneId
  form.syncIntervalSeconds = timeStore.syncIntervalSeconds
}

async function refreshTimeStatus(): Promise<void> {
  loading.value = true
  try {
    const status = await fetchTimeStatus()
    timeStore.replaceFromStatus(status)
    const settings = await fetchTimeSettings()
    timeStore.replaceFromSettings(settings)
    applySettingsToForm()
  } finally {
    loading.value = false
  }
}

async function saveSettings(): Promise<void> {
  saving.value = true
  errorMessage.value = ''
  try {
    const settings = await updateTimeSettings({
      enabled: form.enabled,
      ntpServer: form.ntpServer,
      timezoneId: form.timezoneId,
      syncIntervalSeconds: form.syncIntervalSeconds,
    })
    timeStore.replaceFromSettings(settings)
    applySettingsToForm()
    notifications.notify(t('time.saveSuccess'), 'success')
    await refreshTimeStatus()
  } catch (error) {
    errorMessage.value = error instanceof Error && error.message.length > 0 ? error.message : t('time.saveError')
  } finally {
    saving.value = false
  }
}

async function syncNow(): Promise<void> {
  // An unchanged settings PUT still forces the firmware to resync immediately instead of waiting
  // out the periodic interval - reuse it rather than adding a dedicated endpoint.
  syncingNow.value = true
  try {
    const settings = await updateTimeSettings({})
    timeStore.replaceFromSettings(settings)
    await refreshTimeStatus()
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
  void (async () => {
    const catalog = await fetchTimezones()
    timezones.value = catalog.timezones
    await refreshTimeStatus()
  })()
})
</script>
