import assert from 'node:assert/strict'
import test from 'node:test'

import {
  buildDeviceTypeOptions,
  deviceCategoryOrder,
} from '../../../../../src/components/devices/registry/device-type-options.ts'
import type { DeviceCategory } from '../../../../../src/components/devices/registry/device-ui-types.ts'

const translate = (key: string): string => key

const deviceUis = deviceCategoryOrder.map((category, index) => ({
  category,
  icon: 'device' as const,
  labelKey: `device.type.${category}`,
  searchAliases: category === 'rtc' ? ['real time clock'] : undefined,
  typeId: index + 1,
  typeName: category === 'rtc' ? 'rtc_ds3231' : category,
}))

test('device type options follow the shared category order', () => {
  const options = buildDeviceTypeOptions({
    deviceUis,
    translate,
    valueFor: ui => ui.typeName,
  })

  assert.deepEqual(
    options.filter(option => option.type === 'subheader').map(option => option.title),
    deviceCategoryOrder.map(category => `device.category.${category}`),
  )
  assert.equal(options.filter(option => option.type === 'item').length, deviceUis.length)
})

test('device type options omit categories with no registered devices', () => {
  const options = buildDeviceTypeOptions({
    deviceUis: deviceUis.filter(ui => ui.category === ('buses' satisfies DeviceCategory)),
    translate,
    valueFor: ui => ui.typeName,
  })

  assert.deepEqual(
    options.filter(option => option.type === 'subheader').map(option => option.title),
    ['device.category.buses'],
  )
})

test('device type options include labels, technical names, categories, and aliases in search data', () => {
  const options = buildDeviceTypeOptions({
    deviceUis,
    translate,
    valueFor: ui => ui.typeName,
  })
  const rtcOption = options.find(option => option.type === 'item' && option.title === 'device.type.rtc')

  assert.equal(rtcOption?.type, 'item')
  if (rtcOption?.type !== 'item') {
    return
  }
  assert.match(rtcOption.searchText, /device\.type\.rtc/)
  assert.match(rtcOption.searchText, /rtc_ds3231/)
  assert.match(rtcOption.searchText, /device\.category\.rtc/)
  assert.match(rtcOption.searchText, /real time clock/)
})

test('device type options place an optional all-types action before grouped options', () => {
  const options = buildDeviceTypeOptions({
    deviceUis,
    translate,
    valueFor: ui => ui.typeId,
    firstOption: {
      title: 'All types',
      value: 'all',
    },
  })

  assert.deepEqual(options.slice(0, 2), [
    {
      type: 'item',
      title: 'All types',
      value: 'all',
      searchText: 'All types',
    },
    { type: 'subheader', title: 'device.category.buses' },
  ])
})
