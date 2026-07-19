<template>
  <div class="d-flex flex-wrap align-center ga-2 mt-2">
    <v-chip size="small" color="warning" variant="tonal">
      {{ t('device.fields.sunrise') }} {{ formatAnalogScheduleTime(daylight.sunriseMinute) }}
    </v-chip>
    <v-chip size="small" color="info" variant="tonal">
      {{ t('device.fields.sunset') }} {{ formatAnalogScheduleTime(daylight.sunsetMinute) }}
    </v-chip>
    <v-chip v-if="daylight.approximate" size="small" color="on-surface-variant" variant="outlined">
      {{ t('device.fields.daylightApproximate') }}
    </v-chip>
    <v-chip
      v-if="locationStatus === 'denied' || locationStatus === 'unavailable'"
      size="small"
      color="warning"
      variant="outlined"
    >
      {{
        t(
          locationStatus === 'denied'
            ? 'device.fields.daylightLocationDenied'
            : 'device.fields.daylightLocationUnavailable',
        )
      }}
    </v-chip>
    <v-btn
      size="small"
      variant="text"
      color="primary"
      :loading="locationStatus === 'requesting'"
      @click="$emit('request-location')"
    >
      {{
        t(
          hasCoordinates
            ? 'device.fields.updateDaylightLocation'
            : 'device.fields.useDaylightLocation',
        )
      }}
    </v-btn>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import { formatAnalogScheduleTime } from '@/models/devices/analog-schedule'
import type { AnalogDaylightWindow } from '@/models/devices/analog-daylight'
import type { DaylightLocationStatus } from '@/stores/daylight'

defineProps<{
  daylight: AnalogDaylightWindow
  hasCoordinates: boolean
  locationStatus: DaylightLocationStatus
}>()

defineEmits<{
  'request-location': []
}>()

const { t } = useI18n()
</script>
