<template>
  <PageContainer>
    <PageToolbar :title="t('navigation.overview')" :subtitle="t('overview.copy')" />
    <v-card>
      <v-card-text class="text-medium-emphasis">
        {{ t('overview.copy') }}
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
