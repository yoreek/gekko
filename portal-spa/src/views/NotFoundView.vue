<template>
  <v-container class="not-found" fluid>
    <v-card class="not-found-card">
      <v-card-title>{{ t('notFound.title') }}</v-card-title>
      <v-card-text>{{ t('notFound.body') }}</v-card-text>
    </v-card>
  </v-container>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

const { t } = useI18n()
</script>

<style scoped>
.not-found {
  padding: 48px 24px;
}

.not-found-card {
  padding: 24px;
}
</style>
