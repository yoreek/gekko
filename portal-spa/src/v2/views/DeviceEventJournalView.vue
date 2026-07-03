<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.deviceEvents')" :subtitle="t('journal.subtitle')">
          <template #actions>
            <v-chip color="primary" variant="tonal">
              {{ t('journal.metrics.retained', { count: journalStore.entries.length }) }}
            </v-chip>
          </template>
        </PageToolbar>
      </template>

      <p class="text-body-medium text-medium-emphasis mb-4">
        {{ t('journal.copy') }}
      </p>

      <v-row>
        <v-col cols="12" md="3">
          <v-select v-model="typeFilter" :items="typeFilterOptions" :label="t('journal.filters.type')" density="compact" />
        </v-col>
        <v-col cols="12" md="3">
          <v-select v-model="eventKindFilter" :items="eventKindFilterOptions" :label="t('journal.filters.eventKind')" density="compact" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-select-on-focus v-model="nameFilter" :label="t('journal.filters.name')" density="compact" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-select-on-focus v-model="deviceIdFilter" :label="t('journal.filters.deviceId')" density="compact" inputmode="numeric" />
        </v-col>
      </v-row>
    </PageCard>

    <PageCard class="mt-4">
      <div v-if="journalStore.entries.length === 0" class="text-medium-emphasis pa-4">
        {{ t('journal.empty') }}
      </div>

      <div v-else-if="filteredEntries.length === 0" class="text-medium-emphasis pa-4">
        {{ t('journal.filteredEmpty') }}
      </div>

      <v-data-table
        v-else
        v-model:expanded="expandedSequences"
        :headers="tableHeaders"
        :items="filteredEntries"
        item-value="sequence"
        show-expand
        density="comfortable"
      >
        <template #item.receivedAt="{ item }">
          {{ formatTime(item.receivedAt) }}
        </template>
        <template #item.deviceId="{ item }">
          #{{ item.deviceId }}
        </template>
        <template #item.name="{ item }">
          {{ item.name || t('journal.missingValue') }}
        </template>
        <template #item.typeName="{ item }">
          {{ typeLabel(item.typeName) }}
        </template>
        <template #item.eventKind="{ item }">
          <v-chip color="secondary" variant="tonal">
            {{ eventKindLabel(item.eventKind) }}
          </v-chip>
        </template>
        <template #expanded-row="{ item, columns }">
          <tr>
            <td :colspan="columns.length">
              <v-row class="py-3">
                <v-col cols="6" sm="3">
                  <div class="text-body-small text-medium-emphasis">{{ t('journal.details.topic') }}</div>
                  <div class="text-body-medium">{{ item.topic }}</div>
                </v-col>
                <v-col cols="6" sm="3">
                  <div class="text-body-small text-medium-emphasis">{{ t('journal.details.revision') }}</div>
                  <div class="text-body-medium">{{ item.registryRevision }}</div>
                </v-col>
                <v-col cols="6" sm="3">
                  <div class="text-body-small text-medium-emphasis">{{ t('journal.details.eventKind') }}</div>
                  <div class="text-body-medium">{{ eventKindLabel(item.eventKind) }}</div>
                </v-col>
                <v-col cols="6" sm="3">
                  <div class="text-body-small text-medium-emphasis">{{ t('journal.details.receivedAt') }}</div>
                  <div class="text-body-medium">{{ formatTime(item.receivedAt) }}</div>
                </v-col>
                <v-col cols="12">
                  <pre class="text-body-medium">{{ formatPayload(item.details) }}</pre>
                </v-col>
              </v-row>
            </td>
          </tr>
        </template>
      </v-data-table>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import {
  type DeviceJournalEventKindFilter,
  type DeviceJournalTypeFilter,
  journalEventKindTranslationKey,
  useDeviceEventLogStore,
} from '@/stores/deviceEventLog'
import { allDeviceUisV2, resolveDeviceUiV2 } from '@/v2/components/registry/device-ui-registry'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'
import PageCard from '@/v2/components/layout/PageCard.vue'

const { t, locale } = useI18n()
const journalStore = useDeviceEventLogStore()

const typeFilter = ref<DeviceJournalTypeFilter>('all')
const eventKindFilter = ref<DeviceJournalEventKindFilter>('all')
const nameFilter = ref('')
const deviceIdFilter = ref('')
const expandedSequences = ref<string[]>([])

const filteredEntries = computed(() =>
  journalStore.filteredEntries({
    typeId: typeFilter.value,
    eventKind: eventKindFilter.value,
    name: nameFilter.value,
    deviceId: deviceIdFilter.value,
  }),
)

const tableHeaders = [
  { title: t('journal.columns.receivedAt'), key: 'receivedAt' },
  { title: t('journal.columns.deviceId'), key: 'deviceId' },
  { title: t('journal.columns.name'), key: 'name' },
  { title: t('journal.columns.type'), key: 'typeName' },
  { title: t('journal.columns.eventKind'), key: 'eventKind' },
]

const typeFilterOptions = computed(() => [
  { title: t('journal.filters.allTypes'), value: 'all' },
  ...allDeviceUisV2.map(ui => ({ title: t(ui.labelKey), value: ui.typeId })),
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

function formatTime(receivedAt: number): string {
  return dateFormatter.value.format(new Date(receivedAt))
}

function typeLabel(typeName: string): string {
  const labelKey = resolveDeviceUiV2(typeName).labelKey
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
