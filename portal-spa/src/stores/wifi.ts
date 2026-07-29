import { defineStore } from 'pinia'

import type { WifiScanNetwork, WifiStatusResponse } from '@/api'

export const useWifiStore = defineStore('wifi', {
  state: () => ({
    wifiStatus: 'idle' as WifiStatusResponse['wifiStatus'],
    stationIp: '',
    setupApIp: '',
    bleProvisioningSupported: false,
    scanNetworks: [] as WifiScanNetwork[],
  }),
  actions: {
    replaceStatus(payload: Pick<WifiStatusResponse, 'wifiStatus' | 'stationIp' | 'setupApIp'> & Partial<Pick<WifiStatusResponse, 'bleProvisioningSupported'>>): void {
      this.wifiStatus = payload.wifiStatus
      this.stationIp = payload.stationIp
      this.setupApIp = payload.setupApIp
      if (typeof payload.bleProvisioningSupported === 'boolean') {
        this.bleProvisioningSupported = payload.bleProvisioningSupported
      }
    },
    replaceScan(networks: WifiScanNetwork[]): void {
      this.scanNetworks = [...networks]
    },
  },
})
