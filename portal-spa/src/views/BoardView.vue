<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.board')" :subtitle="t('board.subtitle')" />
      </template>

      <v-progress-linear
        v-if="boardLifecycle.initialLoading.value || boardLifecycle.refreshing.value"
        indeterminate
        color="primary"
      />

      <v-alert
        v-else-if="boardLifecycle.loadError.value !== null && !boardLifecycle.ready.value"
        type="error"
        variant="tonal"
      >
        {{ loadErrorMessage }}
      </v-alert>

      <v-form v-else-if="boardDraft" :disabled="boardLifecycle.busy.value">
        <div class="d-flex flex-column ga-4">
          <v-select
            v-model="boardDraft.selectedBoardId"
            :label="t('board.selectLabel')"
            :items="boardItems"
            item-title="title"
            item-value="value"
          />

          <v-alert v-if="boardLifecycle.saveError.value !== null" type="error" variant="tonal">
            {{ saveErrorMessage }}
          </v-alert>
        </div>
      </v-form>

      <template #actions>
        <v-btn
          :loading="boardLifecycle.saving.value"
          :disabled="!boardLifecycle.canSave.value"
          color="primary"
          size="small"
          @click="saveSettings"
        >
          {{ t('actions.save') }}
        </v-btn>
      </template>
    </PageCard>

    <PageCard v-if="selectedPins.length > 0" class="mt-4">
      <template #header>
        <PageToolbar :title="t('board.pinTableTitle')" :subtitle="selectedBoardLabel" />
      </template>

      <div class="d-flex justify-center mb-4">
        <BoardPinoutDiagram :pins="selectedPins" :layout="selectedLayout" :label="selectedBoardLabel" />
      </div>

      <v-table density="compact">
        <thead>
          <tr>
            <th>{{ t('board.pinTable.gpio') }}</th>
            <th>{{ t('board.pinTable.roles') }}</th>
            <th>{{ t('board.pinTable.note') }}</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="pin in selectedPins" :key="pin.gpio">
            <td>GPIO{{ pin.gpio }}</td>
            <td>
              <v-chip
                v-for="role in pin.roles"
                :key="role"
                size="x-small"
                variant="tonal"
                class="mr-1"
              >
                {{ t(`device.dialog.pinRole.${role}`) }}
              </v-chip>
              <v-chip v-if="pin.fixedDefaultFor" size="x-small" variant="tonal" color="secondary">
                {{ pin.fixedDefaultFor }}
              </v-chip>
            </td>
            <td>{{ pin.note ?? '' }}</td>
          </tr>
        </tbody>
      </v-table>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchBoardSettings, updateBoardSettings } from '@/api'
import { useAsyncForm } from '@/composables/useAsyncForm'
import {
  createBoardSettingsDraft,
  isBoardSettingsDirty,
  type BoardSettingsDraft,
  type BoardSettingsSnapshot,
} from '@/models/board-settings-form'
import { boardPinDetails } from '@/models/devices/shared/pin'
import { BOARD_CATALOG } from '@/data/board-pin-capabilities'
import { useBoardStore } from '@/stores/board'
import { useNotificationsStore } from '@/stores/notifications'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'
import BoardPinoutDiagram from '@/components/devices/common/BoardPinoutDiagram.vue'

const { t } = useI18n()
const boardStore = useBoardStore()
const notifications = useNotificationsStore()

const boardLifecycle = useAsyncForm<BoardSettingsSnapshot, BoardSettingsDraft>({
  load: loadBoardSettingsSnapshot,
  createDraft: createBoardSettingsDraft,
  isDirty: isBoardSettingsDirty,
  save: saveBoardSettingsSnapshot,
  onCommit: commitBoardSettingsSnapshot,
})

const boardDraft = computed(() => boardLifecycle.draft.value)
const loadErrorMessage = computed(() => formatError(boardLifecycle.loadError.value, t('notifications.error')))
const saveErrorMessage = computed(() => formatError(boardLifecycle.saveError.value, t('notifications.error')))

// Only the boards belonging to the chip this firmware actually compiles for -- the SPA's full
// BOARD_CATALOG spans every researched chip, but supportedBoardIds (from the connected device's
// REST response) narrows it to what that device can actually be.
const boardItems = computed(() => {
  const source = boardLifecycle.source.value
  if (!source) return []
  return source.settings.supportedBoardIds.map(boardId => ({
    title: BOARD_CATALOG[boardId]?.label ?? boardId,
    value: boardId,
  }))
})

const selectedBoardId = computed(() => boardDraft.value?.selectedBoardId ?? '')
const selectedBoardLabel = computed(() => BOARD_CATALOG[selectedBoardId.value]?.label ?? '')
const selectedPins = computed(() => (selectedBoardId.value ? boardPinDetails(selectedBoardId.value) : []))
const selectedLayout = computed(() => BOARD_CATALOG[selectedBoardId.value]?.layout)

function formatError(error: unknown, fallback: string): string {
  return error instanceof Error && error.message.length > 0 ? error.message : fallback
}

async function loadBoardSettingsSnapshot(): Promise<BoardSettingsSnapshot> {
  const settings = await fetchBoardSettings()
  return { settings }
}

async function saveBoardSettingsSnapshot(
  { draft }: { source: BoardSettingsSnapshot; draft: BoardSettingsDraft },
): Promise<BoardSettingsSnapshot> {
  const settings = await updateBoardSettings(draft.selectedBoardId)
  return { settings }
}

function commitBoardSettingsSnapshot(source: BoardSettingsSnapshot): void {
  boardStore.replaceFromSettings(source.settings)
}

async function saveSettings(): Promise<void> {
  if (await boardLifecycle.save()) {
    notifications.notify(t('notifications.saved'), 'success')
  }
}

onMounted(() => {
  void boardLifecycle.initialize()
})
</script>
