<template>
  <v-dialog :model-value="modelValue" max-width="560" @update:model-value="$emit('update:modelValue', $event)">
    <v-card>
      <v-card-title class="d-flex align-center ga-2">
        {{ t('device.dialog.dosingPump.history.title') }}
        <v-spacer />
        <v-btn
          icon="download"
          variant="text"
          size="small"
          :disabled="loading || entries.length === 0"
          :aria-label="t('device.dialog.dosingPump.history.export')"
          @click="exportCsv"
        />
        <v-btn icon="refresh" variant="text" size="small" :loading="loading" :aria-label="t('actions.refresh')" @click="reload" />
      </v-card-title>
      <v-card-text class="d-flex flex-column ga-4">
        <v-btn-toggle v-model="periodDays" mandatory="force" divided density="comfortable" @update:model-value="reload">
          <v-btn :value="7">{{ t('device.dialog.dosingPump.history.days', { days: 7 }) }}</v-btn>
          <v-btn :value="30">{{ t('device.dialog.dosingPump.history.days', { days: 30 }) }}</v-btn>
          <v-btn :value="90">{{ t('device.dialog.dosingPump.history.days', { days: 90 }) }}</v-btn>
        </v-btn-toggle>

        <div class="d-flex flex-wrap ga-2">
          <v-chip variant="tonal">{{ t('device.dialog.dosingPump.history.totalDoses', { count: entries.length }) }}</v-chip>
          <v-chip variant="tonal">{{ t('device.dialog.dosingPump.history.totalVolume', { amount: totalVolume }) }}</v-chip>
          <v-chip variant="tonal">{{ t('device.dialog.dosingPump.history.dailyAverage', { amount: dailyAverage }) }}</v-chip>
        </div>

        <v-alert v-if="error" type="error" variant="tonal" density="comfortable">{{ error }}</v-alert>
        <v-alert v-else-if="!loading && entries.length === 0" type="info" variant="tonal" density="comfortable">
          {{ t('device.dialog.dosingPump.history.empty') }}
        </v-alert>

        <v-table v-if="entries.length > 0" density="comfortable">
          <thead>
            <tr>
              <th>{{ t('device.dialog.dosingPump.history.time') }}</th>
              <th>{{ t('device.dialog.dosingPump.history.type') }}</th>
              <th class="text-end">{{ t('device.dialog.dosingPump.history.amount') }}</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(entry, index) in entries" :key="index">
              <td>{{ formatLocalFlavoredEpoch(entry.at, locale === 'ru' ? 'ru-RU' : 'en-US') }}</td>
              <td>
                <v-chip size="small" variant="tonal" :color="entry.type === 'manual' ? 'warning' : 'info'">
                  {{ t(entry.type === 'manual' ? 'device.dialog.dosingPump.history.manual' : 'device.dialog.dosingPump.history.scheduled') }}
                </v-chip>
              </td>
              <td class="text-end">{{ entry.amountMl.toFixed(2) }} {{ t('device.dialog.dosingPump.ml') }}</td>
            </tr>
          </tbody>
        </v-table>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="$emit('update:modelValue', false)">{{ t('actions.close') }}</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, DoseJournalEntry } from '@/api/contracts'
import { fetchDoseJournal } from '@/api/doseJournal'
import { formatLocalFlavoredEpoch } from '@/models/devices/dosing-pump-math'

const props = defineProps<{
  modelValue: boolean
  device: DeviceRecord
}>()

defineEmits<{
  'update:modelValue': [value: boolean]
}>()

const { t, locale } = useI18n()

const periodDays = ref(7)
const entries = ref<DoseJournalEntry[]>([])
const loading = ref(false)
const error = ref('')

const totalVolume = computed(() => Math.round(entries.value.reduce((sum, entry) => sum + entry.amountMl, 0) * 100) / 100)
const dailyAverage = computed(() => Math.round((totalVolume.value / periodDays.value) * 100) / 100)

async function reload(): Promise<void> {
  loading.value = true
  error.value = ''
  try {
    const response = await fetchDoseJournal(props.device.record.id, periodDays.value)
    entries.value = response.entries
  } catch (loadError) {
    entries.value = []
    error.value = loadError instanceof Error ? loadError.message : String(loadError)
  } finally {
    loading.value = false
  }
}

function formatCsvTimestamp(epochSeconds: number): string {
  return new Date(epochSeconds * 1000).toISOString().slice(0, 19).replace('T', ' ')
}

function exportCsv(): void {
  const lines = ['time,type,amount_ml']
  for (const entry of entries.value) {
    lines.push(`${formatCsvTimestamp(entry.at)},${entry.type},${entry.amountMl.toFixed(2)}`)
  }
  const stamp = new Date().toISOString().slice(0, 10).replaceAll('-', '')
  const blob = new Blob([lines.join('\n') + '\n'], { type: 'text/csv;charset=utf-8' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = `dose-journal-${props.device.record.id}-${periodDays.value}d-${stamp}.csv`
  link.click()
  URL.revokeObjectURL(url)
}

watch(
  () => props.modelValue,
  open => {
    if (open) {
      void reload()
    }
  },
)
</script>
