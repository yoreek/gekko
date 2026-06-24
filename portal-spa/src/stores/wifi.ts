import { defineStore } from 'pinia'

import type { WifiScanNetwork, WifiStatusResponse } from '@/api'

export const useWifiStore = defineStore('wifi', {
  state: () => ({
    wifiStatus: 'idle' as WifiStatusResponse['wifiStatus'],
    stationIp: '',
    setupApIp: '',
    scanNetworks: [] as WifiScanNetwork[],
  }),
  actions: {
    replaceStatus(payload: Pick<WifiStatusResponse, 'wifiStatus' | 'stationIp' | 'setupApIp'>): void {
      this.wifiStatus = payload.wifiStatus
      this.stationIp = payload.stationIp
      this.setupApIp = payload.setupApIp
    },
    replaceScan(networks: WifiScanNetwork[]): void {
      this.scanNetworks = [...networks]
    },
  },
})
