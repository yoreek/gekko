<template>
  <div class="device-dialog__section-copy">{{ t('device.dialog.dummyCompact') }}</div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

const { t } = useI18n()
</script>

<style scoped>
.device-dialog__section-copy {
  color: rgb(var(--v-theme-on-surface));
}
</style>
