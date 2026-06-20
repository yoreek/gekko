export type DummyDeviceConfigDraft = Record<string, never>

export function createDefaultDummyDeviceConfig(): DummyDeviceConfigDraft {
  return {}
}
