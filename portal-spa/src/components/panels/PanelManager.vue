<template>
  <div class="panel-manager">
    <v-card class="page-card" elevation="1">
      <v-card-title class="page-title">
        <div>
          <div class="eyebrow">{{ t('panels.title') }}</div>
          <h2>{{ t('panels.subtitle') }}</h2>
        </div>
      </v-card-title>

      <v-card-text>
        <div class="panel-manager__hint">
          {{ t('panels.uniqueHint') }}
        </div>

        <div v-if="panelStore.panels.length > 0" class="stack mt-4">
          <v-card
            v-for="panel in panelStore.panels"
            :key="panel.id"
            class="panel-manager__item"
            :class="{ 'panel-manager__item--active': panel.id === panelStore.activePanelId }"
            variant="outlined"
          >
            <v-card-text class="panel-manager__row">
              <div class="panel-manager__main">
                <v-text-field
                  :model-value="panelNameDrafts[panel.id] ?? panel.name"
                  :label="t('panels.name')"
                  density="comfortable"
                  hide-details
                  @update:model-value="value => updateDraft(panel.id, String(value))"
                />
              </div>
              <div class="panel-manager__actions">
                <v-btn icon size="small" variant="text" :disabled="panelStore.panels.indexOf(panel) === 0" @click="movePanel(panel.id, -1)">
                  <AppIcon name="chevron-left" />
                </v-btn>
                <v-btn
                  icon
                  size="small"
                  variant="text"
                  :disabled="panelStore.panels.indexOf(panel) === panelStore.panels.length - 1"
                  @click="movePanel(panel.id, 1)"
                >
                  <AppIcon name="chevron-right" />
                </v-btn>
                <v-btn size="small" variant="tonal" @click="savePanelName(panel.id)">
                  {{ t('panels.rename') }}
                </v-btn>
                <v-btn size="small" variant="tonal" @click="activatePanel(panel.id)">
                  {{ t('panels.activate') }}
                </v-btn>
                <v-btn size="small" color="error" variant="text" :disabled="panelStore.panels.length <= 1" @click="deletePanel(panel.id)">
                  <AppIcon name="trash" />
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

import AppIcon from '@/components/AppIcon.vue'
import { usePanelStore } from '@/stores/panels'

const { t } = useI18n()
const panelStore = usePanelStore()
const panelNameDrafts = reactive<Record<number, string>>({})

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

function updateDraft(panelId: number, value: string): void {
  panelNameDrafts[panelId] = value
}

function savePanelName(panelId: number): void {
  panelStore.renamePanel(panelId, panelNameDrafts[panelId] ?? '')
  const panel = panelStore.panels.find(entry => entry.id === panelId)
  if (panel) {
    panelNameDrafts[panelId] = panel.name
  }
}

function deletePanel(panelId: number): void {
  panelStore.deletePanel(panelId)
}

function movePanel(panelId: number, direction: -1 | 1): void {
  panelStore.movePanel(panelId, direction)
}

function activatePanel(panelId: number): void {
  panelStore.setActivePanel(panelId)
}
</script>
