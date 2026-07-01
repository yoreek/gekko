<template>
  <v-row no-gutters align="center" class="ga-3 mb-4">
    <v-col v-if="showBack" cols="auto">
      <v-btn icon="chevron-left" variant="text" :aria-label="t('actions.cancel')" @click="$emit('back')" />
    </v-col>
    <v-col>
      <div role="heading" aria-level="2" class="text-h6 font-weight-bold text-truncate">{{ title }}</div>
      <div v-if="subtitle" class="text-body-2 text-medium-emphasis text-truncate">{{ subtitle }}</div>
    </v-col>
    <v-col v-if="$slots.actions" cols="auto">
      <div class="d-flex ga-2 flex-wrap justify-end">
        <slot name="actions" />
      </div>
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

withDefaults(defineProps<{
  title: string
  subtitle?: string
  showBack?: boolean
}>(), {
  showBack: false,
})

defineEmits<{ back: [] }>()

const { t } = useI18n()
</script>
