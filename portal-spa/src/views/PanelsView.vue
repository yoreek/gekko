<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.panels')" :subtitle="t('panels.subtitle')">
          <template #actions>
            <v-btn color="primary" prepend-icon="plus" @click="openCreatePanelDialog">
              {{ t('dashboard.addPanel') }}
            </v-btn>
          </template>
        </PageToolbar>
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
          variant="outlined"
          color="on-surface"
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
                <v-chip v-if="panel.id === panelStore.activePanelId" class="me-2" color="primary" size="small" variant="tonal">
                  {{ t('panels.active') }}
                </v-chip>
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

    <v-dialog v-model="panelDialogOpen" max-width="420">
      <v-card>
        <v-card-item>
          <v-card-title>{{ t('dashboard.addPanel') }}</v-card-title>
          <v-card-subtitle>{{ t('dashboard.addPanelHint') }}</v-card-subtitle>
        </v-card-item>
        <v-card-text>
          <v-text-field v-select-on-focus v-model="panelNameDraft" :label="t('dashboard.panelName')" autofocus />
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="panelDialogOpen = false">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn color="primary" :disabled="panelNameDraft.trim().length === 0" @click="submitCreatePanel">
            {{ t('dashboard.addPanel') }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </PageContainer>
</template>

<script setup lang="ts">
import { onBeforeMount, reactive, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore } from '@/stores/panels'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()
const panelNameDrafts = reactive<Record<string, string>>({})
const panelDialogOpen = ref(false)
const panelNameDraft = ref('')

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

function openCreatePanelDialog(): void {
  panelNameDraft.value = `Panel ${panelStore.panels.length + 1}`
  panelDialogOpen.value = true
}

function submitCreatePanel(): void {
  const panel = panelStore.addPanel(panelNameDraft.value)
  if (panel) {
    panelDialogOpen.value = false
  }
}
</script>
