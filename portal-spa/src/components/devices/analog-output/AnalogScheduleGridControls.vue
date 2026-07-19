<template>
  <v-row class="align-center">
    <v-col cols="12" sm="4">
      <v-text-field
        v-model="timeStep"
        type="number"
        min="5"
        max="240"
        :label="t('device.fields.scheduleGridTimeStep')"
        suffix="min"
        density="compact"
        hide-details
        :disabled="!enabled"
        @blur="$emit('normalize')"
      />
    </v-col>
    <v-col cols="12" sm="4">
      <v-text-field
        v-model="levelStep"
        type="number"
        min="1"
        max="100"
        :label="t('device.fields.scheduleGridLevelStep')"
        suffix="%"
        density="compact"
        hide-details
        :disabled="!enabled"
        @blur="$emit('normalize')"
      />
    </v-col>
    <v-col cols="12" sm="4">
      <v-checkbox
        v-model="enabled"
        :label="t('device.fields.scheduleGridEnabled')"
        density="compact"
        hide-details
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

const timeStep = defineModel<string>('timeStep', { required: true })
const levelStep = defineModel<string>('levelStep', { required: true })
const enabled = defineModel<boolean>('enabled', { required: true })

defineEmits<{
  normalize: []
}>()

const { t } = useI18n()
</script>
