<template>
  <v-container class="page-shell" fluid>
    <v-card class="page-card page-hero" elevation="2">
      <v-card-title class="page-title">
        <div>
          <div class="eyebrow">{{ t('panels.title') }}</div>
          <h1>{{ t('panels.subtitle') }}</h1>
        </div>
      </v-card-title>
      <v-card-text>
        <p class="hero-copy">{{ t('panels.copy') }}</p>
      </v-card-text>
    </v-card>

    <PanelManager class="mt-4" />
  </v-container>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchDevices } from '@/api'
import PanelManager from '@/components/panels/PanelManager.vue'
import { usePanelStore } from '@/stores/panels'

const { t } = useI18n()
const panelStore = usePanelStore()

onMounted(async () => {
  const response = await fetchDevices()
  panelStore.initialize(response.devices.map(device => device.device_id))
  panelStore.syncDeviceIds(response.devices.map(device => device.device_id))
})
</script>
