<template>
  <v-container class="page-shell" fluid>
    <v-card class="page-card page-hero">
      <v-card-title class="page-title">
        <div>
          <div class="text-overline">{{ t('journal.title') }}</div>
          <h1 class="text-h4 font-weight-bold">{{ t('journal.subtitle') }}</h1>
        </div>
        <v-chip color="primary" variant="tonal">
          {{ t('journal.metrics.retained', { count: journalStore.entries.length }) }}
        </v-chip>
      </v-card-title>
      <v-card-text>
        <p class="text-body-1">
          {{ t('journal.copy') }}
        </p>

        <v-row class="journal-filters">
          <v-col cols="12" md="3">
            <v-select
              v-model="typeFilter"
              :items="typeFilterOptions"
              :label="t('journal.filters.type')"
            />
          </v-col>
          <v-col cols="12" md="3">
            <v-select
              v-model="eventKindFilter"
              :items="eventKindFilterOptions"
              :label="t('journal.filters.eventKind')"
            />
          </v-col>
          <v-col cols="12" md="3">
            <v-text-field
              v-select-on-focus
              v-model="nameFilter"
              :label="t('journal.filters.name')"
            />
          </v-col>
          <v-col cols="12" md="3">
            <v-text-field
              v-select-on-focus
              v-model="deviceIdFilter"
              :label="t('journal.filters.deviceId')"
              inputmode="numeric"
            />
          </v-col>
        </v-row>
      </v-card-text>
    </v-card>

    <v-card class="page-card mt-4">
      <v-card-text>
        <div v-if="journalStore.entries.length === 0" class="journal-empty text-body-1">
          {{ t('journal.empty') }}
        </div>

        <div v-else-if="filteredEntries.length === 0" class="journal-empty text-body-1">
          {{ t('journal.filteredEmpty') }}
        </div>

        <v-table v-else class="journal-table">
          <thead>
            <tr>
              <th class="journal-table__expand-column"></th>
              <th>{{ t('journal.columns.receivedAt') }}</th>
              <th>{{ t('journal.columns.deviceId') }}</th>
              <th>{{ t('journal.columns.name') }}</th>
              <th>{{ t('journal.columns.type') }}</th>
              <th>{{ t('journal.columns.eventKind') }}</th>
            </tr>
          </thead>
          <tbody>
            <template v-for="entry in filteredEntries" :key="entry.sequence">
              <tr class="journal-table__row">
                <td>
                  <v-btn
                    class="journal-table__toggle"
                    :icon="isExpanded(entry.sequence) ? 'chevron-down' : 'chevron-right'"
                    variant="text"
                    :aria-label="isExpanded(entry.sequence) ? t('journal.actions.collapse') : t('journal.actions.expand')"
                    @click="toggleEntry(entry.sequence)"
                  />
                </td>
                <td class="journal-table__time">{{ formatTime(entry.receivedAt) }}</td>
                <td>#{{ entry.deviceId }}</td>
                <td>{{ entry.name || t('journal.missingValue') }}</td>
                <td>{{ typeLabel(entry.typeName) }}</td>
                <td>
                  <v-chip color="secondary" variant="tonal">
                    {{ eventKindLabel(entry.eventKind) }}
                  </v-chip>
                </td>
              </tr>
              <tr v-if="isExpanded(entry.sequence)" class="journal-table__details-row">
                <td :colspan="6">
                  <div class="journal-details">
                    <div class="journal-details__grid">
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
                    </div>
                    <pre class="journal-details__payload">{{ formatPayload(entry.details) }}</pre>
                  </div>
                </td>
              </tr>
            </template>
          </tbody>
        </v-table>
      </v-card-text>
    </v-card>
  </v-container>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { allDeviceUis, resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import {
  type DeviceJournalEventKindFilter,
  type DeviceJournalTypeFilter,
  journalEventKindTranslationKey,
  useDeviceEventLogStore,
} from '@/stores/deviceEventLog'

const { t, locale } = useI18n()
const journalStore = useDeviceEventLogStore()

const typeFilter = ref<DeviceJournalTypeFilter>('all')
const eventKindFilter = ref<DeviceJournalEventKindFilter>('all')
const nameFilter = ref('')
const deviceIdFilter = ref('')
const expandedSequences = ref<number[]>([])

const filteredEntries = computed(() =>
  journalStore.filteredEntries({
    typeId: typeFilter.value,
    eventKind: eventKindFilter.value,
    name: nameFilter.value,
    deviceId: deviceIdFilter.value,
  }),
)

const typeFilterOptions = computed(() => [
  { title: t('journal.filters.allTypes'), value: 'all' },
  ...allDeviceUis.map(ui => ({ title: t(ui.labelKey), value: ui.typeId })),
])

const eventKindFilterOptions = computed(() => [
  { title: t('journal.filters.allEventKinds'), value: 'all' },
  { title: t('journal.eventKinds.deviceCreated'), value: 'device_created' },
  { title: t('journal.eventKinds.deviceUpdated'), value: 'device_updated' },
  { title: t('journal.eventKinds.statusChanged'), value: 'status_changed' },
  { title: t('journal.eventKinds.stateChanged'), value: 'state_changed' },
  { title: t('journal.eventKinds.configPersisted'), value: 'config_persisted' },
  { title: t('journal.eventKinds.retainedStateChanged'), value: 'retained_state_changed' },
  { title: t('journal.eventKinds.deviceDeleted'), value: 'device_deleted' },
  { title: t('journal.eventKinds.commandAccepted'), value: 'command_accepted' },
  { title: t('journal.eventKinds.commandRejected'), value: 'command_rejected' },
  { title: t('journal.eventKinds.snapshot'), value: 'snapshot' },
])

const dateFormatter = computed(() => {
  const localeTag = locale.value === 'ru' ? 'ru-RU' : 'en-US'
  return new Intl.DateTimeFormat(localeTag, {
    dateStyle: 'medium',
    timeStyle: 'medium',
  })
})

function toggleEntry(sequence: number): void {
  if (expandedSequences.value.includes(sequence)) {
    expandedSequences.value = expandedSequences.value.filter(entry => entry !== sequence)
    return
  }
  expandedSequences.value = [...expandedSequences.value, sequence]
}

function isExpanded(sequence: number): boolean {
  return expandedSequences.value.includes(sequence)
}

function formatTime(receivedAt: number): string {
  return dateFormatter.value.format(new Date(receivedAt))
}

function typeLabel(typeName: string): string {
  const labelKey = resolveDeviceUi(typeName).labelKey
  return labelKey === 'device.type.unknown' && typeName.length > 0 ? typeName : t(labelKey)
}

function eventKindLabel(eventKind: string): string {
  const translationKey = journalEventKindTranslationKey(eventKind)
  return translationKey === null ? eventKind : t(translationKey)
}

function formatPayload(payload: Record<string, unknown>): string {
  return JSON.stringify(payload, null, 2)
}
</script>

<style scoped>
.journal-filters {
  margin-top: 4px;
}

.journal-empty {
  padding: 28px 8px;
}

.journal-table__expand-column {
  width: 44px;
}

.journal-table__toggle {
  min-width: 36px;
  width: 36px;
  height: 36px;
}

.journal-table__time {
  white-space: nowrap;
  font-variant-numeric: tabular-nums;
}

.journal-table__details-row td {
  padding-top: 0;
}

.journal-details {
  display: grid;
  gap: 12px;
  padding: 12px 0 8px;
}

.journal-details__grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px 16px;
}

.journal-details__payload {
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

@media (max-width: 900px) {
  .journal-details__grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 600px) {
  .journal-details__grid {
    grid-template-columns: 1fr;
  }
}
</style>
