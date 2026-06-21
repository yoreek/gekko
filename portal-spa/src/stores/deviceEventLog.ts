import { defineStore } from 'pinia'

import type { RealtimeMessage } from '@/realtime/messages'

export type DeviceJournalTypeFilter = number | 'all'
export type DeviceJournalEventKindFilter = string | 'all'

export interface DeviceEventJournalEntry {
  sequence: number
  receivedAt: number
  topic: string
  revision: number
  deviceId: number
  typeId: number
  typeName: string
  name: string
  eventKind: string
  registryRevision: number
  configRevision: number | null
  pendingPersistence: boolean | null
  details: Record<string, unknown>
  raw: RealtimeMessage<Record<string, unknown>>
}

export interface DeviceEventJournalFilters {
  typeId: DeviceJournalTypeFilter
  eventKind: DeviceJournalEventKindFilter
  name: string
  deviceId: string
}

interface DeviceEventPayload extends Record<string, unknown> {
  event_kind?: unknown
  device_id?: unknown
  type_id?: unknown
  type?: unknown
  label?: unknown
  name?: unknown
  registry_revision?: unknown
  config_revision?: unknown
  pending_persistence?: unknown
  device?: unknown
}

const kMaxEntries = 200

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function asNumber(value: unknown): number | null {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? numeric : null
}

function asText(value: unknown): string {
  return typeof value === 'string' ? value.trim() : ''
}

function extractPayload(message: RealtimeMessage): DeviceEventPayload | null {
  if (!isRecord(message.payload)) {
    return null
  }
  return message.payload as DeviceEventPayload
}

function extractSnapshot(payload: DeviceEventPayload): DeviceEventPayload | null {
  if (isRecord(payload.device)) {
    return payload.device as DeviceEventPayload
  }
  return payload
}

export function journalEventKindTranslationKey(eventKind: string): string | null {
  switch (eventKind) {
    case 'device_created':
      return 'journal.eventKinds.deviceCreated'
    case 'device_updated':
      return 'journal.eventKinds.deviceUpdated'
    case 'status_changed':
      return 'journal.eventKinds.statusChanged'
    case 'state_changed':
      return 'journal.eventKinds.stateChanged'
    case 'config_persisted':
      return 'journal.eventKinds.configPersisted'
    case 'retained_state_changed':
      return 'journal.eventKinds.retainedStateChanged'
    case 'device_deleted':
      return 'journal.eventKinds.deviceDeleted'
    case 'command_accepted':
      return 'journal.eventKinds.commandAccepted'
    case 'command_rejected':
      return 'journal.eventKinds.commandRejected'
    case 'snapshot':
      return 'journal.eventKinds.snapshot'
    default:
      return null
  }
}

function createEntry(message: RealtimeMessage, receivedAt: number): DeviceEventJournalEntry | null {
  if (message.topic !== 'device.upsert' && message.topic !== 'device.command_result' && message.topic !== 'device.remove') {
    return null
  }

  const payload = extractPayload(message)
  if (payload === null) {
    return null
  }

  const snapshot = extractSnapshot(payload)
  const eventKind = asText(payload.event_kind)
  if (snapshot === null) {
    return null
  }

  const deviceId = asNumber(snapshot.device_id ?? payload.device_id)
  const typeId = asNumber(snapshot.type_id ?? payload.type_id)
  if (deviceId === null || deviceId <= 0 || typeId === null || typeId <= 0) {
    return null
  }

  const name = asText(snapshot.name ?? payload.name)
  const typeName = asText(snapshot.type ?? payload.type ?? payload.label)
  const registryRevision = asNumber(payload.registry_revision ?? message.revision) ?? message.revision
  const configRevision = asNumber(snapshot.config_revision ?? payload.config_revision)
  const pendingPersistenceValue = payload.pending_persistence
  const pendingPersistence = typeof pendingPersistenceValue === 'boolean' ? pendingPersistenceValue : null

  return {
    sequence: 0,
    receivedAt,
    topic: message.topic,
    revision: message.revision,
    deviceId,
    typeId,
    typeName,
    name,
    eventKind,
    registryRevision,
    configRevision,
    pendingPersistence,
    details: payload,
    raw: {
      topic: message.topic,
      revision: message.revision,
      payload: payload,
    },
  }
}

export const useDeviceEventLogStore = defineStore('deviceEventLog', {
  state: () => ({
    entries: [] as DeviceEventJournalEntry[],
    nextSequence: 1,
  }),
  getters: {
    hasEntries: state => state.entries.length > 0,
  },
  actions: {
    append(message: RealtimeMessage, receivedAt = Date.now()): DeviceEventJournalEntry | null {
      const entry = createEntry(message, receivedAt)
      if (entry === null) {
        return null
      }

      entry.sequence = this.nextSequence++
      this.entries.unshift(entry)
      if (this.entries.length > kMaxEntries) {
        this.entries.length = kMaxEntries
      }
      return entry
    },
    clear(): void {
      this.entries = []
      this.nextSequence = 1
    },
    filteredEntries(filters: DeviceEventJournalFilters): DeviceEventJournalEntry[] {
      const nameQuery = filters.name.trim().toLowerCase()
      const exactDeviceId = filters.deviceId.trim().length > 0 ? Number(filters.deviceId.trim()) : null
      return this.entries.filter(entry => {
        const matchesType = filters.typeId === 'all' || entry.typeId === filters.typeId
        const matchesEventKind = filters.eventKind === 'all' || entry.eventKind === filters.eventKind
        const matchesName = nameQuery.length === 0 || entry.name.toLowerCase().includes(nameQuery)
        const matchesDeviceId = exactDeviceId === null ? true : Number.isInteger(exactDeviceId) && entry.deviceId === exactDeviceId
        return matchesType && matchesEventKind && matchesName && matchesDeviceId
      })
    },
    latestEntriesForDevice(deviceId: number, limit = 5): DeviceEventJournalEntry[] {
      if (!Number.isInteger(deviceId) || deviceId <= 0 || limit <= 0) {
        return []
      }
      return this.entries.filter(entry => entry.deviceId === deviceId).slice(0, limit)
    },
  },
})
