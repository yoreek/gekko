<template>
  <PageContainer>
    <PageToolbar :title="t('navigation.deviceEvents')" :subtitle="t('journal.subtitle')" />
    <v-card>
      <v-card-text class="text-medium-emphasis">
        Event journal placeholder
      </v-card-text>
    </v-card>
  </PageContainer>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'

const { t } = useI18n()
</script>
