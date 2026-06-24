import { defineStore } from 'pinia'

import type { DeviceRecord } from '@/api'
import type { RealtimeMessage } from '@/realtime/messages'
import { deviceTypeIdFromName } from '../models/device-types'

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
  details: Record<string, unknown>
  raw: RealtimeMessage<Record<string, unknown>>
}

export interface DeviceEventJournalFilters {
  typeId: DeviceJournalTypeFilter
  eventKind: DeviceJournalEventKindFilter
  name: string
  deviceId: string
}

const kMaxEntries = 200

function asNumber(value: unknown): number | null {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? numeric : null
}

function asText(value: unknown): string {
  return typeof value === 'string' ? value.trim() : ''
}

function eventKindFromTopic(topic: string): string {
  switch (topic) {
    case 'device.upsert':
      return 'device_updated'
    case 'device.command_result':
      return 'command_accepted'
    case 'device.remove':
      return 'device_deleted'
    default:
      return ''
  }
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

  const payload = message.payload as DeviceRecord & {
    record?: { id?: unknown; typeName?: unknown; configRevision?: unknown }
    config?: { name?: unknown }
    eventKind?: unknown
    registryRevision?: unknown
  }
  const record = payload.record ?? {}
  const config = payload.config ?? {}
  const eventKind = asText(payload.eventKind) || eventKindFromTopic(message.topic)
  const deviceId = asNumber(record.id)
  const typeName = asText(record.typeName)
  const typeId = asNumber(typeName.length > 0 ? deviceTypeIdFromName(typeName) : null)
  if (deviceId === null || deviceId <= 0 || typeId === null || typeId <= 0) {
    return null
  }

  const name = asText(config.name)
  const registryRevision = asNumber(payload.registryRevision ?? message.revision) ?? message.revision
  const configRevision = asNumber(record.configRevision)
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
    details: payload as unknown as Record<string, unknown>,
    raw: message as RealtimeMessage<Record<string, unknown>>,
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
