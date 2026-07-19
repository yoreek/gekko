<template>
  <v-menu location="bottom end">
    <template #activator="{ props: menuProps }">
      <v-btn
        v-bind="menuProps"
        class="app-bar__icon-button"
        variant="text"
        icon
        :aria-label="t('notifications.alerts.title')"
      >
        <v-badge v-if="alerts.length > 0" :content="alerts.length" :color="alertsStore.hasCritical ? 'error' : 'warning'">
          <v-icon icon="bell" />
        </v-badge>
        <v-icon v-else icon="bell" />
      </v-btn>
    </template>
    <v-list min-width="280" max-width="400">
      <v-list-item v-if="alerts.length === 0">
        <v-list-item-title class="text-medium-emphasis">{{ t('notifications.alerts.empty') }}</v-list-item-title>
      </v-list-item>
      <v-list-item
        v-for="alert in alerts"
        :key="alert.id"
        :to="{ name: 'device-detail', params: { id: alert.deviceId } }"
      >
        <template #prepend>
          <v-icon icon="alert" :color="alert.severity === 'critical' ? 'error' : 'warning'" />
        </template>
        <v-list-item-title>{{ alert.deviceName }}</v-list-item-title>
        <v-list-item-subtitle>{{ t(alert.messageKey, alert.messageParams ?? {}) }}</v-list-item-subtitle>
      </v-list-item>
    </v-list>
  </v-menu>
</template>

<script setup lang="ts">
import { computed, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceAlertSeverity } from '@/components/devices/registry/device-ui-types'
import { useDeviceAlertsStore } from '@/stores/deviceAlerts'
import { useNotificationsStore } from '@/stores/notifications'

const { t } = useI18n()
const alertsStore = useDeviceAlertsStore()
const notificationsStore = useNotificationsStore()

const alerts = computed(() => alertsStore.activeAlerts)

// Toast once per alert appearance (and again on warning -> critical escalation);
// the standing entry in the bell menu covers everything after that.
let seenSeverities = new Map<string, DeviceAlertSeverity>()

watch(
  alerts,
  current => {
    for (const alert of current) {
      const previous = seenSeverities.get(alert.id)
      if (previous === undefined || (previous === 'warning' && alert.severity === 'critical')) {
        notificationsStore.notify(
          `${alert.deviceName}: ${t(alert.messageKey, alert.messageParams ?? {})}`,
          alert.severity === 'critical' ? 'error' : 'warning',
        )
      }
    }
    seenSeverities = new Map(current.map(alert => [alert.id, alert.severity]))
  },
  { immediate: true },
)
</script>
