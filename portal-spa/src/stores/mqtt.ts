import { defineStore } from 'pinia'

import type { MqttSettingsRecord, MqttStatusResponse } from '@/api'

export const useMqttStore = defineStore('mqtt', {
  state: () => ({
    enabled: false,
    connected: false,
    waitingForStation: false,
    host: '',
    port: 1883,
    useTls: false,
    clientId: '',
    username: '',
    haDiscoveryPrefix: 'homeassistant',
    haNodeId: '',
    haNodeName: '',
    hasCaCert: false,
    settingsEnabled: false,
  }),
  actions: {
    replaceFromStatus(payload: MqttStatusResponse): void {
      this.enabled = payload.enabled
      this.connected = payload.connected
      this.waitingForStation = payload.waitingForStation
      this.host = payload.host
      this.port = payload.port
      this.useTls = payload.useTls
      this.clientId = payload.clientId
      this.hasCaCert = payload.hasCaCert
    },
    replaceFromSettings(payload: MqttSettingsRecord): void {
      this.settingsEnabled = payload.enabled
      this.host = payload.host
      this.port = payload.port
      this.useTls = payload.useTls
      this.clientId = payload.clientId
      this.username = payload.username
      this.haDiscoveryPrefix = payload.haDiscoveryPrefix
      this.haNodeId = payload.haNodeId
      this.haNodeName = payload.haNodeName
      this.hasCaCert = payload.hasCaCert ?? this.hasCaCert
    },
  },
})
