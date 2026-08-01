<template>
  <v-tabs color="primary" density="comfortable" show-arrows>
    <v-tab :to="{ name: 'dashboard' }">
      {{ t('dashboard.views.panels') }}
    </v-tab>
    <v-tab :to="{ name: 'device-graph' }">
      {{ t('dashboard.views.dependencies') }}
    </v-tab>
  </v-tabs>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

const { t } = useI18n()
</script>
