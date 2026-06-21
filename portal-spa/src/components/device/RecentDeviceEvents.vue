<template>
  <v-expansion-panels class="recent-device-events" variant="accordion" flat>
    <v-expansion-panel>
      <v-expansion-panel-title class="recent-device-events__title">
        <div class="recent-device-events__title-copy">
          <AppIcon name="journal" />
          <div>
            <div class="text-subtitle-1 font-weight-medium">{{ t('device.dialog.recentEventsTitle') }}</div>
            <div class="text-body-2 text-medium-emphasis">{{ t('device.dialog.recentEventsSubtitle') }}</div>
          </div>
        </div>
      </v-expansion-panel-title>
      <v-expansion-panel-text>
        <v-expansion-panels v-if="entries.length > 0" class="recent-device-events__entries" variant="accordion" flat>
          <v-expansion-panel v-for="entry in entries" :key="entry.sequence" class="recent-device-events__entry">
            <v-expansion-panel-title class="recent-device-events__entry-title">
              <div class="recent-device-events__summary">
                <v-chip class="recent-device-events__time" size="small" variant="tonal">
                  {{ formatTime(entry.receivedAt) }}
                </v-chip>
                <div class="recent-device-events__summary-copy">
                  <div class="text-body-2 text-medium-emphasis">
                    #{{ entry.deviceId }} · {{ typeLabel(entry.typeId) }} · {{ eventKindLabel(entry.eventKind) }}
                  </div>
                </div>
              </div>
            </v-expansion-panel-title>

            <v-expansion-panel-text>
              <div class="recent-device-events__details">
                <div class="recent-device-events__grid">
                  <div>
                    <div class="text-caption text-medium-emphasis">{{ t('journal.details.topic') }}</div>
                    <div class="text-body-2">{{ entry.topic }}</div>
                  </div>
                  <div>
                    <div class="text-caption text-medium-emphasis">{{ t('journal.details.revision') }}</div>
                    <div class="text-body-2">{{ entry.registryRevision }}</div>
                  </div>
                  <div>
                    <div class="text-caption text-medium-emphasis">{{ t('journal.details.eventKind') }}</div>
                    <div class="text-body-2">{{ eventKindLabel(entry.eventKind) }}</div>
                  </div>
                  <div>
                    <div class="text-caption text-medium-emphasis">{{ t('journal.details.receivedAt') }}</div>
                    <div class="text-body-2">{{ formatTime(entry.receivedAt) }}</div>
                  </div>
                  <div>
                    <div class="text-caption text-medium-emphasis">{{ t('journal.details.pendingPersistence') }}</div>
                    <div class="text-body-2">{{ formatPendingPersistence(entry.pendingPersistence) }}</div>
                  </div>
                </div>
                <pre class="recent-device-events__payload">{{ formatPayload(entry.details) }}</pre>
              </div>
            </v-expansion-panel-text>
          </v-expansion-panel>
        </v-expansion-panels>

        <div v-else class="text-body-2 text-medium-emphasis">
          {{ t('device.dialog.recentEventsEmpty') }}
        </div>
      </v-expansion-panel-text>
    </v-expansion-panel>
  </v-expansion-panels>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import AppIcon from '@/components/AppIcon.vue'
import { deviceTypeLabelKey } from '@/models/device-types'
import {
  journalEventKindTranslationKey,
  useDeviceEventLogStore,
} from '@/stores/deviceEventLog'

const props = defineProps<{
  deviceId: number
}>()

const { t, locale } = useI18n()
const deviceEventLogStore = useDeviceEventLogStore()

const entries = computed(() => deviceEventLogStore.latestEntriesForDevice(props.deviceId, 5))
const dateFormatter = computed(() => {
  const localeTag = locale.value === 'ru' ? 'ru-RU' : 'en-US'
  return new Intl.DateTimeFormat(localeTag, {
    dateStyle: 'short',
    timeStyle: 'medium',
  })
})

function formatTime(receivedAt: number): string {
  return dateFormatter.value.format(new Date(receivedAt))
}

function typeLabel(typeId: number): string {
  return t(deviceTypeLabelKey(typeId))
}

function eventKindLabel(eventKind: string): string {
  const translationKey = journalEventKindTranslationKey(eventKind)
  return translationKey === null ? eventKind : t(translationKey)
}

function formatPendingPersistence(pendingPersistence: boolean | null): string {
  if (pendingPersistence === null) {
    return t('journal.missingValue')
  }
  return pendingPersistence ? t('labels.yes') : t('labels.no')
}

function formatPayload(payload: Record<string, unknown>): string {
  return JSON.stringify(payload, null, 2)
}
</script>

<style scoped>
.recent-device-events {
  margin-top: 8px;
}

.recent-device-events__title-copy {
  display: flex;
  align-items: center;
  gap: 12px;
  min-width: 0;
}

.recent-device-events__entries {
  display: grid;
  grid-template-columns: minmax(0, 1fr);
  gap: 8px;
  width: 100%;
}

.recent-device-events__entry {
  width: 100%;
  border-top: 1px solid rgb(var(--v-theme-outline));
}

.recent-device-events__entry-title {
  width: 100%;
  align-items: flex-start;
}

.recent-device-events__entry-title :deep(.v-expansion-panel-title__content) {
  width: 100%;
  min-width: 0;
}

.recent-device-events__entry-title :deep(.v-expansion-panel-title__spacer) {
  width: 0;
  display: none;
}

.recent-device-events__entry-title :deep(.v-expansion-panel-title__icon) {
  flex: 0 0 auto;
}

.recent-device-events__entry-title :deep(.v-expansion-panel-title__prepend) {
  flex: 0 0 auto;
}

.recent-device-events__summary {
  width: 100%;
}

.recent-device-events__summary {
  display: flex;
  align-items: flex-start;
  gap: 12px;
  min-width: 0;
}

.recent-device-events__summary-copy {
  flex: 1 1 auto;
  min-width: 0;
}

.recent-device-events__grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px 16px;
}

.recent-device-events__details {
  display: grid;
  gap: 12px;
}

.recent-device-events__payload {
  margin: 0;
  padding: 12px;
  border-radius: 8px;
  overflow: auto;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  background: rgb(var(--v-theme-surface));
  color: rgb(var(--v-theme-on-surface));
  font-size: 0.82rem;
  line-height: 1.4;
}

.recent-device-events__time {
  font-variant-numeric: tabular-nums;
}

@media (max-width: 600px) {
  .recent-device-events__grid {
    grid-template-columns: 1fr;
  }
}
</style>
