import type { MqttSettingsRecord, MqttStatusResponse } from '@/api'

export interface MqttSettingsSnapshot {
  status: MqttStatusResponse
  settings: MqttSettingsRecord | null
}

export type MqttSaveOperation = 'settings' | 'replace-certificate' | 'remove-certificate'

export type MqttSettingsDraft =
  | {
      available: false
    }
  | {
      available: true
      enabled: boolean
      host: string
      port: number
      useTls: boolean
      clientId: string
      username: string
      password: string
      haDiscoveryPrefix: string
      haNodeId: string
      haNodeName: string
      caCertAction: 'keep' | 'replace' | 'remove'
      caCertFile: File | null
    }

const editableSettingKeys = [
  'enabled',
  'host',
  'port',
  'useTls',
  'clientId',
  'username',
  'haDiscoveryPrefix',
  'haNodeId',
  'haNodeName',
] as const

export function createMqttSettingsDraft(snapshot: MqttSettingsSnapshot): MqttSettingsDraft {
  if (snapshot.settings === null) {
    return { available: false }
  }
  return {
    available: true,
    enabled: snapshot.settings.enabled,
    host: snapshot.settings.host,
    port: snapshot.settings.port,
    useTls: snapshot.settings.useTls,
    clientId: snapshot.settings.clientId,
    username: snapshot.settings.username,
    password: '',
    haDiscoveryPrefix: snapshot.settings.haDiscoveryPrefix,
    haNodeId: snapshot.settings.haNodeId,
    haNodeName: snapshot.settings.haNodeName,
    caCertAction: 'keep',
    caCertFile: null,
  }
}

export function areMqttSettingFieldsDirty(snapshot: MqttSettingsSnapshot, draft: MqttSettingsDraft): boolean {
  if (snapshot.settings === null || !draft.available) {
    return false
  }
  return draft.password.length > 0
    || editableSettingKeys.some(key => draft[key] !== snapshot.settings?.[key])
}

export function isMqttSettingsDirty(snapshot: MqttSettingsSnapshot, draft: MqttSettingsDraft): boolean {
  return areMqttSettingFieldsDirty(snapshot, draft)
    || (draft.available && draft.caCertAction !== 'keep')
}

export function planMqttSaveOperations(
  snapshot: MqttSettingsSnapshot,
  draft: MqttSettingsDraft,
): MqttSaveOperation[] {
  if (snapshot.settings === null || !draft.available) {
    return []
  }
  const operations: MqttSaveOperation[] = []
  if (areMqttSettingFieldsDirty(snapshot, draft)) {
    operations.push('settings')
  }
  if (draft.caCertAction === 'replace' && draft.caCertFile !== null) {
    operations.push('replace-certificate')
  } else if (draft.caCertAction === 'remove') {
    operations.push('remove-certificate')
  }
  return operations
}

export function buildMqttSettingsPayload(draft: MqttSettingsDraft): Partial<MqttSettingsRecord> {
  if (!draft.available) {
    return {}
  }
  const payload: Partial<MqttSettingsRecord> = {
    enabled: draft.enabled,
    host: draft.host,
    port: draft.port,
    useTls: draft.useTls,
    clientId: draft.clientId,
    username: draft.username,
    haDiscoveryPrefix: draft.haDiscoveryPrefix,
    haNodeId: draft.haNodeId,
    haNodeName: draft.haNodeName,
  }
  if (draft.password.length > 0) {
    payload.password = draft.password
  }
  return payload
}

export function applyMqttSettingsToStatus(
  current: MqttStatusResponse,
  settings: MqttSettingsRecord,
  hasCaCert: boolean,
): MqttStatusResponse {
  return {
    ...current,
    host: settings.host,
    port: settings.port,
    useTls: settings.useTls,
    clientId: settings.clientId,
    hasCaCert,
  }
}
