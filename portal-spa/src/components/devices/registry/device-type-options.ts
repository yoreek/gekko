import type { PortalIconName } from '@/icons'

import type { DeviceCategory, DeviceUi } from './device-ui-types'

export const deviceCategoryOrder = [
  'buses',
  'expanders',
  'temperatureSensors',
  'inputs',
  'outputs',
  'controllers',
  'rtc',
  'automation',
  'displays',
  'service',
] as const satisfies readonly DeviceCategory[]

export const deviceTypeFilterKeys: string[] = ['title', 'raw.searchText']

export type DeviceTypeOptionValue = string | number | null
export type DeviceTypeOptionSource =
  Pick<DeviceUi, 'category' | 'icon' | 'labelKey' | 'searchAliases' | 'typeId' | 'typeName'>

export interface DeviceTypeOption<T extends DeviceTypeOptionValue> {
  type: 'item'
  title: string
  value: T
  searchText: string
  props?: {
    prependIcon: PortalIconName
  }
}

export interface DeviceTypeSubheader {
  type: 'subheader'
  title: string
}

export type DeviceTypeListItem<T extends DeviceTypeOptionValue> =
  | DeviceTypeOption<T>
  | DeviceTypeSubheader

interface BuildDeviceTypeOptionsParams<T extends DeviceTypeOptionValue> {
  deviceUis: readonly DeviceTypeOptionSource[]
  translate: (key: string) => string
  valueFor: (ui: DeviceTypeOptionSource) => T
  firstOption?: {
    title: string
    value: T
  }
}

export function buildDeviceTypeOptions<T extends DeviceTypeOptionValue>({
  deviceUis,
  translate,
  valueFor,
  firstOption,
}: BuildDeviceTypeOptionsParams<T>): DeviceTypeListItem<T>[] {
  const items: DeviceTypeListItem<T>[] = []

  if (firstOption) {
    items.push({
      type: 'item',
      title: firstOption.title,
      value: firstOption.value,
      searchText: firstOption.title,
    })
  }

  for (const category of deviceCategoryOrder) {
    const categoryTitle = translate(`device.category.${category}`)
    const categoryDevices = deviceUis.filter(ui => ui.category === category)
    if (categoryDevices.length === 0) {
      continue
    }

    items.push({ type: 'subheader', title: categoryTitle })
    items.push(
      ...categoryDevices.map(ui => {
        const title = translate(ui.labelKey)
        return {
          type: 'item' as const,
          title,
          value: valueFor(ui),
          searchText: [title, ui.typeName, categoryTitle, ...(ui.searchAliases ?? [])].join(' '),
          props: {
            prependIcon: ui.icon,
          },
        }
      }),
    )
  }

  return items
}
