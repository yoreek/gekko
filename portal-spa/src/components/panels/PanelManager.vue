<template>
  <div class="panel-manager">
    <v-card class="page-card">
      <v-card-text>
        <div class="panel-manager__hint text-body-2 mt-2">
          {{ t('panels.uniqueHint') }}
        </div>

        <div v-if="panelStore.panels.length > 0" class="stack mt-4">
          <v-card
            v-for="panel in panelStore.panels"
            :key="panel.id"
            class="panel-manager__item"
            :class="{ 'panel-manager__item--active': panel.id === panelStore.activePanelId }"
          >
            <v-card-text class="panel-manager__row">
              <div class="panel-manager__main">
                <v-text-field
                  v-select-on-focus
                  :model-value="panelNameDrafts[panel.id] ?? panel.name"
                  :label="t('panels.name')"
                  @update:model-value="value => updateDraft(panel.id, String(value))"
                />
              </div>
              <div class="panel-manager__actions">
                <v-btn icon="chevron-left" variant="text" :disabled="panelStore.panels.indexOf(panel) === 0" @click="movePanel(panel.id, -1)" />
                <v-btn
                  icon="chevron-right"
                  variant="text"
                  :disabled="panelStore.panels.indexOf(panel) === panelStore.panels.length - 1"
                  @click="movePanel(panel.id, 1)"
                />
                <v-btn variant="tonal" @click="savePanelName(panel.id)">
                  {{ t('panels.rename') }}
                </v-btn>
                <v-btn variant="tonal" @click="activatePanel(panel.id)">
                  {{ t('panels.activate') }}
                </v-btn>
                <v-btn color="error" variant="text" :disabled="panelStore.panels.length <= 1" @click="deletePanel(panel.id)">
                  <v-icon class="me-1" icon="trash" />
                  {{ t('panels.delete') }}
                </v-btn>
              </div>
            </v-card-text>
          </v-card>
        </div>

        <div v-else class="empty-state">
          {{ t('panels.empty') }}
        </div>
      </v-card-text>
    </v-card>
  </div>
</template>

<script setup lang="ts">
import { reactive, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { usePanelStore } from '@/stores/panels'

const { t } = useI18n()
const panelStore = usePanelStore()
const panelNameDrafts = reactive<Record<string, string>>({})

watch(
  () => panelStore.panels.map(panel => `${panel.id}:${panel.name}`).join('|'),
  () => {
    for (const panel of panelStore.panels) {
      if (!(panel.id in panelNameDrafts)) {
        panelNameDrafts[panel.id] = panel.name
      }
    }
  },
  { immediate: true },
)

function updateDraft(panelId: string, value: string): void {
  panelNameDrafts[panelId] = value
}

function savePanelName(panelId: string): void {
  panelStore.renamePanel(panelId, panelNameDrafts[panelId] ?? '')
  const panel = panelStore.panels.find(entry => entry.id === panelId)
  if (panel) {
    panelNameDrafts[panelId] = panel.name
  }
}

function deletePanel(panelId: string): void {
  panelStore.deletePanel(panelId)
}

function movePanel(panelId: string, direction: -1 | 1): void {
  panelStore.movePanel(panelId, direction)
}

function activatePanel(panelId: string): void {
  panelStore.setActivePanel(panelId)
}
</script>

<style scoped>
.panel-manager {
  display: grid;
  gap: 18px;
}

.panel-manager__hint {
  margin-top: 8px;
}

.panel-manager__item--active {
  border-color: rgb(var(--v-theme-primary));
}

.panel-manager__row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
}

.panel-manager__main {
  flex: 1 1 320px;
  min-width: 0;
}

.panel-manager__actions {
  display: flex;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
}
</style>
