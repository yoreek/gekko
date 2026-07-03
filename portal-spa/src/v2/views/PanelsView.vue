<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.panels')" :subtitle="t('panels.subtitle')" />
      </template>

      <p class="text-body-medium text-medium-emphasis mb-4">
        {{ t('panels.uniqueHint') }}
      </p>

      <div v-if="panelStore.panels.length === 0" class="text-medium-emphasis">
        {{ t('panels.empty') }}
      </div>

      <div v-else class="d-flex flex-column ga-3">
        <v-card
          v-for="(panel, index) in panelStore.panels"
          :key="panel.id"
          :variant="panel.id === panelStore.activePanelId ? 'tonal' : 'outlined'"
          :color="panel.id === panelStore.activePanelId ? 'primary' : undefined"
        >
          <v-card-text>
            <v-row align="center" class="ga-0">
              <v-col cols="12" sm="6">
                <v-text-field
                  v-select-on-focus
                  :model-value="panelNameDrafts[panel.id] ?? panel.name"
                  :label="t('panels.name')"
                  density="compact"
                  hide-details
                  @update:model-value="value => updateDraft(panel.id, String(value))"
                />
              </v-col>
              <v-col cols="12" sm="6" class="d-flex flex-wrap align-center justify-sm-end ga-1">
                <v-btn icon="chevron-left" variant="text" size="small" :disabled="index === 0" :aria-label="t('panels.moveLeft')" @click="panelStore.movePanel(panel.id, -1)" />
                <v-btn icon="chevron-right" variant="text" size="small" :disabled="index === panelStore.panels.length - 1" :aria-label="t('panels.moveRight')" @click="panelStore.movePanel(panel.id, 1)" />
                <v-btn variant="tonal" size="small" @click="savePanelName(panel.id)">
                  {{ t('panels.rename') }}
                </v-btn>
                <v-btn variant="tonal" size="small" :disabled="panel.id === panelStore.activePanelId" @click="panelStore.setActivePanel(panel.id)">
                  {{ t('panels.activate') }}
                </v-btn>
                <v-btn color="error" variant="text" size="small" :disabled="panelStore.panels.length <= 1" @click="panelStore.deletePanel(panel.id)">
                  <v-icon class="me-1" icon="trash" />
                  {{ t('panels.delete') }}
                </v-btn>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </div>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { onBeforeMount, reactive, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore } from '@/stores/panels'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'
import PageCard from '@/v2/components/layout/PageCard.vue'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()
const panelNameDrafts = reactive<Record<string, string>>({})

onBeforeMount(async () => {
  await deviceStore.initialize()
  await panelStore.initialize(deviceStore.devices.map(device => device.record.id))
})

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
</script>
