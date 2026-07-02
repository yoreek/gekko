<template>
  <v-snackbar
    :model-value="current !== null"
    :color="current?.color"
    location="top"
    :timeout="current?.timeout ?? 4000"
    @update:model-value="onModelChange"
  >
    {{ current?.message }}
    <template #actions>
      <v-btn variant="text" @click="dismiss">
        {{ t('actions.close') }}
      </v-btn>
    </template>
  </v-snackbar>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { useNotificationsStore } from '@/v2/stores/notifications'

const store = useNotificationsStore()
const { t } = useI18n()

const current = computed(() => store.queue[0] ?? null)

function dismiss(): void {
  if (current.value) store.dismiss(current.value.id)
}

function onModelChange(value: boolean): void {
  if (!value) dismiss()
}
</script>
