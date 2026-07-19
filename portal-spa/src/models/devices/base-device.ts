import type { BaseDeviceConfig, DeviceDependencyLink, DeviceRecord, DeviceCreateRequest } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceCommandRequest } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'

export interface DeviceCreatePayload<TConfig extends BaseDeviceConfig = BaseDeviceConfig> extends DeviceCreateRequest<TConfig> {}

function configValueEquals(a: unknown, b: unknown): boolean {
  if (a === b) return true
  if (typeof a !== 'object' || a === null || typeof b !== 'object' || b === null) return false
  return JSON.stringify(a) === JSON.stringify(b)
}

// Order-independent comparison of two dependency-link snapshots, so re-deriving the same set of
// links in a different order is never mistaken for a real dependency change.
function dependencyLinksEqual(a: DeviceDependencyLink[], b: DeviceDependencyLink[]): boolean {
  if (a.length !== b.length) return false
  const linkKey = (link: DeviceDependencyLink): string => `${link.role}:${link.deviceId}`
  const sortedA = a.map(linkKey).sort()
  const sortedB = b.map(linkKey).sort()
  return sortedA.every((value, index) => value === sortedB[index])
}

// Deep-clones a plain JSON-safe value. Config objects/arrays passed in here are frequently live
// Vue-reactive proxies (from a store's DeviceRecord), and structuredClone throws on those
// ("could not be cloned") -- round-tripping through JSON is what actually unwraps a reactive
// proxy into a plain, independent copy, since JSON.stringify reads through proxy traps.
function deepCloneJson<T>(value: T): T {
  return JSON.parse(JSON.stringify(value)) as T
}

// The base fields every device config carries. Concrete types' static defaultConfig() should
// spread this instead of re-declaring name/enabled/deps by hand.
export function defaultBaseDeviceConfig(): BaseDeviceConfig {
  return {
    name: 'New Device',
    enabled: true,
    deps: [],
  }
}

// The base-field portion of a static normalizeConfig(). Concrete types' static normalizeConfig()
// should spread this and only parse their own extra fields, rather than re-declaring the
// name/enabled/deps parsing rules in every device type.
export function normalizeBaseDeviceConfig(raw: Record<string, unknown>, defaults: BaseDeviceConfig): BaseDeviceConfig {
  return {
    name: typeof raw.name === 'string' ? raw.name : defaults.name,
    enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
    deps: Array.isArray(raw.deps) ? (raw.deps as DeviceDependencyLink[]) : defaults.deps,
  }
}

// The base-field portion of a static encodeConfig(). Types with a custom static encodeConfig
// (real per-field transforms) should spread this instead of re-listing name/enabled/deps, so the
// deps array is always an independent copy rather than an alias of the live (possibly
// Vue-reactive) config.
export function encodeBaseDeviceConfig(config: BaseDeviceConfig): Record<string, unknown> {
  return deepCloneJson({ name: config.name, enabled: config.enabled, deps: config.deps })
}

export abstract class BaseDevice<
  TConfig extends object = Record<string, unknown>,
  TCreateDraft extends DeviceCreateDraftBase & object = DeviceCreateDraftBase & object,
  TOutput extends object = Record<string, unknown>,
> {
  abstract readonly typeName: string
  abstract readonly typeId: number
  // Which dependency role(s) (mirroring the firmware's DeviceRole wire names) this device type can
  // fill, so picker components can list every compatible type for a role instead of hardcoding one
  // type id per dependency field. Empty means it provides none, matching the firmware's empty
  // DeviceTypeDescriptor::providedRoles default. Most types provide exactly one role; a few (e.g. a
  // switch also usable as an AND-condition source) provide two.
  readonly dependencyRoles: DeviceRole[] = []

  abstract createDefaultConfig(): TConfig

  abstract createDefaultCreateDraft(common?: Partial<DeviceCreateDraftBase>): TCreateDraft

  abstract createEditDraft(current: DeviceRecord): TCreateDraft

  abstract normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): TConfig

  abstract normalizeOutput(record: DeviceRecord): TOutput

  // Default edit-command builder: a plain field-diff against the current config, plus a
  // dependency-link diff derived from the same createCreateDeps() every device type already
  // implements for create payloads. `deps` is only attached to the command when the dependency
  // snapshot actually changed -- never unconditionally -- matching the documented partial-update
  // contract (docs/rest-api-contract.md). Device types whose command shape genuinely differs
  // (e.g. a dedicated setDeps command instead of bundling deps into updateConfig) override this;
  // everything else, dependency-having or not, should rely on this default rather than
  // re-declaring the identical dependency-diff body.
  buildEditCommands(current: DeviceRecord, draft: TCreateDraft): DeviceCommandRequest[] {
    const currentConfig = this.normalizeConfig(current.config, current.config.deps)
    const nextDeps = this.createCreateDeps(draft)
    const nextConfig = this.normalizeConfig({ ...draft, name: draft.name.trim() }, nextDeps)
    const configDiff = this.buildConfigDiff(this.encodeConfig(nextConfig), this.encodeConfig(currentConfig))
    // Derived from currentConfig via the same createCreateDeps() projection as nextDeps -- not
    // the raw persisted current.config.deps -- so both sides of the comparison share the same
    // shape (e.g. a type that always projects one link per role, even a zero/unassigned one).
    // Comparing against the raw snapshot directly would flag a shape difference as a real change.
    const currentDeps = this.createCreateDeps(currentConfig as unknown as TCreateDraft)
    const depsChanged = !dependencyLinksEqual(nextDeps, currentDeps)
    const commands: DeviceCommandRequest[] = []
    if (Object.keys(configDiff).length > 0 || depsChanged) {
      const command: DeviceCommandRequest = { command: 'updateConfig', config: configDiff }
      if (depsChanged) {
        command.deps = nextDeps
      }
      commands.push(command)
    }
    return commands
  }

  // Default wire-encoding for a config: a deep clone, never the live (possibly Vue-reactive)
  // object reference. Device types with a 1:1 field mapping (no renaming/rounding/nested
  // re-encoding) should rely on this default rather than re-declaring every field by hand. Types
  // that DO need to transform or drop a field should build on top of it via
  // `super.encodeConfig(config)` rather than starting from the raw config object, so the result
  // is always an independent copy.
  protected encodeConfig(config: TConfig): Record<string, unknown> {
    return deepCloneJson(config) as Record<string, unknown>
  }

  buildCreatePayload(draft: TCreateDraft): DeviceCreatePayload {
    const config = this.extractCreateConfig(draft)
    const payload: DeviceCreatePayload = {
      typeName: this.typeName,
      config: {
        ...(this.encodeConfig(config) as unknown as Record<string, unknown>),
        name: draft.name.trim(),
        enabled: draft.enabled,
        deps: this.createCreateDeps(draft),
      } as BaseDeviceConfig,
    }
    return payload
  }

  // The only supported way to change a device's persisted config. Callers (edit dialogs,
  // dashboard quick-actions, anything else) provide the current record plus whatever fields
  // changed; this normalizes/encodes/diffs against the current config and reuses the same
  // buildEditCommands() every device type already implements -- so command shape, deps
  // handling, and partial-diff behavior stay identical everywhere and are never duplicated
  // per call site. Components must never call encodeConfig/normalizeConfig/buildConfigDiff
  // directly to build an outgoing command.
  buildQuickUpdateCommands(current: DeviceRecord, partial: Partial<TCreateDraft>): DeviceCommandRequest[] {
    const draft = { ...this.createEditDraft(current), ...partial } as TCreateDraft
    return this.buildEditCommands(current, draft)
  }

  protected buildConfigDiff(
    nextEncoded: Record<string, unknown>,
    currentEncoded: Record<string, unknown>,
  ): Record<string, unknown> {
    const diff: Record<string, unknown> = {}
    for (const key of Object.keys(nextEncoded)) {
      if (!configValueEquals(nextEncoded[key], currentEncoded[key])) {
        diff[key] = nextEncoded[key]
      }
    }
    return diff
  }

  // Default create-config extractor: the draft minus its typeName (the one field that belongs
  // to the create envelope, never to the persisted config). Device types whose draft needs no
  // other narrowing should rely on this default; types that must ignore the draft entirely
  // (fixed/stub configs) override it.
  protected extractCreateConfig(draft: TCreateDraft): TConfig {
    const { typeName: _typeName, ...config } = draft
    return config as unknown as TConfig
  }

  protected createCreateDeps(_draft: TCreateDraft): DeviceDependencyLink[] {
    return []
  }
}
