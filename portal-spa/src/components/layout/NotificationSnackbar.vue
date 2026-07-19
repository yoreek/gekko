<template>
  <v-snackbar-queue
    v-model="messages"
    location="top"
    :total-visible="5"
    closable
    :close-text="t('actions.close')"
  />
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { useNotificationsStore } from '@/stores/notifications'

const store = useNotificationsStore()
const { t } = useI18n()

const messages = computed({
  get: () => store.queue,
  set: value => {
    store.queue = value
  },
})
</script>
