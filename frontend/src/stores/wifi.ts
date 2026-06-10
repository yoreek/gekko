import { defineStore } from 'pinia'

import type { WifiScanNetwork, WifiStatusResponse } from '@/api'

export const useWifiStore = defineStore('wifi', {
  state: () => ({
    wifiStatus: 'idle' as WifiStatusResponse['wifi_status'],
    stationIp: '',
    setupApIp: '',
    scanNetworks: [] as WifiScanNetwork[],
  }),
  actions: {
    replaceStatus(payload: Pick<WifiStatusResponse, 'wifi_status' | 'station_ip' | 'setup_ap_ip'>): void {
      this.wifiStatus = payload.wifi_status
      this.stationIp = payload.station_ip
      this.setupApIp = payload.setup_ap_ip
    },
    replaceScan(networks: WifiScanNetwork[]): void {
      this.scanNetworks = [...networks]
    },
  },
})

