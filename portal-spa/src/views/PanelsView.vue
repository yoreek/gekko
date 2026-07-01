<template>
  <v-container class="page-shell" fluid>
    <v-card class="page-card page-hero">
      <v-card-title class="page-title">
        <div>
          <div class="text-overline">{{ t('panels.title') }}</div>
          <h1 class="text-h5 sm:text-h4 font-weight-bold text-wrap">{{ t('panels.subtitle') }}</h1>
        </div>
      </v-card-title>
      <v-card-text>
        <p class="text-body-1">{{ t('panels.copy') }}</p>
      </v-card-text>
    </v-card>

    <PanelManager class="mt-4" />
  </v-container>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import PanelManager from '@/components/panels/PanelManager.vue'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore } from '@/stores/panels'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()

onMounted(async () => {
  await deviceStore.initialize()
  await panelStore.initialize(deviceStore.devices.map(device => device.record.id))
})
</script>
