import {
  mdiChevronDown as chevronDownPath,
  mdiChevronLeft as chevronLeftPath,
  mdiChevronRight as chevronRightPath,
  mdiCheckboxBlankOutline as checkboxBlankOutlinePath,
  mdiCheckboxMarked as checkboxMarkedPath,
  mdiMinusBox as checkboxIndeterminatePath,
  mdiClose as closePath,
  mdiCancel as cancelPath,
  mdiContentCopy as contentCopyPath,
  mdiDownload as downloadPath,
  mdiEye as eyePath,
  mdiEyeOff as eyeOffPath,
  mdiInformationOutline as infoPath,
  mdiMenu as menuPath,
  mdiMinus as minusPath,
  mdiPencil as editPath,
  mdiImageOutline as imageOutlinePath,
  mdiPlus as plusPath,
  mdiRadioboxBlank as radioBlankPath,
  mdiRadioboxMarked as radioMarkedPath,
  mdiPower as powerPath,
  mdiPowerOff as powerOffPath,
  mdiFire as firePath,
  mdiSnowflake as snowflakePath,
  mdiCheckCircleOutline as checkCircleOutlinePath,
  mdiAlertOutline as alertOutlinePath,
  mdiRefresh as refreshPath,
  mdiUpload as uploadPath,
  mdiTrashCan as trashPath,
} from '@mdi/js'

export type PortalIconName =
  | 'portal'
  | 'wifi'
  | 'device'
  | 'bus'
  | 'temperature'
  | 'devices'
  | 'panels'
  | 'ota'
  | 'system'
  | 'ws'
  | 'journal'
  | 'locale'
  | 'sun'
  | 'moon'
  | 'layers'
  | 'oled-text'
  | 'oled-icon'
  | 'oled-bitmap'
  | 'oled-rect'
  | 'oled-line'
  | 'oled-circle'
  | 'oled-ellipse'
  | 'oled-page'
  | 'oled-layer-up'
  | 'oled-layer-down'
  | 'oled-duplicate'
  | 'design-display'
  | 'checkboxOn'
  | 'checkboxOff'
  | 'checkboxIndeterminate'
  | 'radioOn'
  | 'radioOff'
  | 'refresh'
  | 'close'
  | 'edit'
  | 'eye'
  | 'eye-off'
  | 'info'
  | 'trash'
  | 'menu'
  | 'plus'
  | 'minus'
  | 'power'
  | 'power-off'
  | 'flame'
  | 'snowflake'
  | 'check-circle'
  | 'alert'
  | 'download'
  | 'copy'
  | 'upload'
  | 'chevron-left'
  | 'chevron-right'
  | 'chevron-down'

type IconShape = {
  viewBox: string
  paths: string[]
  filled: boolean
}

type IconSource = string | string[]

function createIconShape(paths: IconSource, viewBox = '0 0 24 24'): IconShape {
  return {
    viewBox,
    paths: Array.isArray(paths) ? paths : [paths],
    filled: false,
  }
}

function createFilledIconShape(path: string, viewBox = '0 0 24 24'): IconShape {
  return {
    viewBox,
    paths: [path],
    filled: true,
  }
}

const localIconRegistry: Record<
  | 'portal'
  | 'wifi'
  | 'device'
  | 'bus'
  | 'temperature'
  | 'devices'
  | 'panels'
  | 'ota'
  | 'system'
  | 'ws'
  | 'journal'
  | 'locale'
  | 'sun'
  | 'moon'
  | 'layers'
  | 'oled-text'
  | 'oled-icon'
  | 'oled-bitmap'
  | 'oled-rect'
  | 'oled-line'
  | 'oled-circle'
  | 'oled-ellipse'
  | 'oled-page'
  | 'oled-layer-up'
  | 'oled-layer-down'
  | 'oled-duplicate'
  | 'design-display'
  | 'checkboxOn'
  | 'checkboxOff'
  | 'checkboxIndeterminate'
  | 'radioOn'
  | 'radioOff'
  | 'clear'
  | 'cancel'
  ,
  IconShape
> = {
  portal: createIconShape(['M4 5h16v14H4z', 'M8 9h3v3H8z', 'M13 9h3v3h-3z', 'M8 14h8']),
  wifi: createIconShape(['M4 9a12 12 0 0116 0', 'M7.5 12.5a7 7 0 019 0', 'M10.5 16a3 3 0 013 0', 'M12 19h.01']),
  device: createIconShape(['M7 4h10v16H7z', 'M10 7h4', 'M10 17h4', 'M9 10h6v4H9z']),
  bus: createIconShape(['M6 5h12v14H6z', 'M9 8h6', 'M9 12h6', 'M9 16h3', 'M15 16h0.01']),
  temperature: createIconShape(['M10 4a2 2 0 114 0v8.2a4 4 0 11-4 0z', 'M12 14v3', 'M12 4v8']),
  devices: createIconShape(['M4 5h6v6H4z', 'M14 5h6v6h-6z', 'M4 13h6v6H4z', 'M14 13h6v6h-6z', 'M10 8h4', 'M10 16h4']),
  panels: createIconShape(['M4 5h5v14H4z', 'M11 5h9v5h-9z', 'M11 12h9v7h-9z']),
  ota: createIconShape(['M7 16h10a4 4 0 00.5-7.97A6 6 0 006.3 9.6 3.5 3.5 0 007 16z', 'M12 16V8', 'M9 11l3-3 3 3']),
  system: createIconShape(['M12 8a4 4 0 100 8 4 4 0 000-8z', 'M12 3v3', 'M12 18v3', 'M4.8 6.2l2.1 2.1', 'M17.1 15.7l2.1 2.1', 'M3 12h3', 'M18 12h3', 'M4.8 17.8l2.1-2.1', 'M17.1 8.3l2.1-2.1']),
  journal: createIconShape(['M6 4h10l2 2v14H6z', 'M16 4v4h4', 'M8 10h6', 'M8 14h6', 'M8 18h4']),
  ws: createIconShape(['M6 8h12', 'M6 16h12', 'M8 6a2 2 0 110 4 2 2 0 010-4z', 'M16 14a2 2 0 110 4 2 2 0 010-4z', 'M16 6a2 2 0 110 4 2 2 0 010-4z', 'M8 14a2 2 0 110 4 2 2 0 010-4z']),
  locale: createIconShape(['M12 4a8 8 0 100 16 8 8 0 000-16z', 'M4 12h16', 'M12 4a12 12 0 010 16', 'M12 4a12 12 0 000 16']),
  sun: createIconShape(['M12 8a4 4 0 100 8 4 4 0 000-8z', 'M12 3v2', 'M12 19v2', 'M5 12H3', 'M21 12h-2', 'M6.3 6.3L4.9 4.9', 'M19.1 19.1l-1.4-1.4', 'M6.3 17.7l-1.4 1.4', 'M19.1 4.9l-1.4 1.4']),
  moon: createIconShape(['M19 15.4A7.8 7.8 0 018.6 5a8 8 0 1010.4 10.4z']),
  layers: createIconShape(['M12 5l8 4-8 4-8-4 8-4z', 'M4 12l8 4 8-4', 'M4 16l8 4 8-4']),
  'oled-text': createIconShape(['M5 6h14', 'M12 6v12', 'M7 18h10']),
  'oled-icon': createIconShape(['M12 5l3 6 6 1-4.5 4.3 1.2 6.2L12 18.9 5.3 22.5l1.2-6.2L2 12l6-1z']),
  'oled-bitmap': createIconShape([imageOutlinePath]),
  'oled-rect': createIconShape(['M5 6h14v12H5z']),
  'oled-line': createIconShape(['M5 12h14']),
  'oled-circle': createIconShape(['M12 5a7 7 0 100 14 7 7 0 000-14z']),
  'oled-ellipse': createIconShape(['M4 12a8 5 0 1016 0 8 5 0 10-16 0z']),
  'oled-page': createIconShape(['M6 4h9l3 3v13H6z', 'M15 4v4h4']),
  'oled-layer-up': createIconShape(['M12 4l6 6H6z', 'M6 18h12']),
  'oled-layer-down': createIconShape(['M6 6h12l-6 6z', 'M6 18h12']),
  'oled-duplicate': createIconShape(['M8 8h10v10H8z', 'M6 6h10']),
  'design-display': createFilledIconShape(editPath),
  checkboxOn: createFilledIconShape(checkboxMarkedPath),
  checkboxOff: createIconShape(checkboxBlankOutlinePath),
  checkboxIndeterminate: createFilledIconShape(checkboxIndeterminatePath),
  radioOn: createFilledIconShape(radioMarkedPath),
  radioOff: createIconShape(radioBlankPath),
  clear: createFilledIconShape(cancelPath),
  cancel: createFilledIconShape(cancelPath),
}

const mdiIconRegistry: Record<
  | 'refresh'
  | 'close'
  | 'edit'
  | 'eye'
  | 'eye-off'
  | 'info'
  | 'trash'
  | 'menu'
  | 'plus'
  | 'minus'
  | 'power'
  | 'power-off'
  | 'flame'
  | 'snowflake'
  | 'check-circle'
  | 'alert'
  | 'download'
  | 'copy'
  | 'upload'
  | 'chevron-left'
  | 'chevron-right'
  | 'chevron-down',
  IconShape
> = {
  refresh: createFilledIconShape(refreshPath),
  close: createFilledIconShape(closePath),
  edit: createFilledIconShape(editPath),
  eye: createFilledIconShape(eyePath),
  'eye-off': createFilledIconShape(eyeOffPath),
  info: createFilledIconShape(infoPath),
  trash: createFilledIconShape(trashPath),
  menu: createFilledIconShape(menuPath),
  plus: createFilledIconShape(plusPath),
  minus: createFilledIconShape(minusPath),
  power: createFilledIconShape(powerPath),
  'power-off': createFilledIconShape(powerOffPath),
  flame: createFilledIconShape(firePath),
  snowflake: createFilledIconShape(snowflakePath),
  'check-circle': createFilledIconShape(checkCircleOutlinePath),
  alert: createFilledIconShape(alertOutlinePath),
  download: createFilledIconShape(downloadPath),
  copy: createFilledIconShape(contentCopyPath),
  upload: createFilledIconShape(uploadPath),
  'chevron-left': createFilledIconShape(chevronLeftPath),
  'chevron-right': createFilledIconShape(chevronRightPath),
  'chevron-down': createFilledIconShape(chevronDownPath),
}

export const iconRegistry: Record<PortalIconName, IconShape> = {
  ...localIconRegistry,
  ...mdiIconRegistry,
}

export function resolveIconShape(name: string | undefined | null): IconShape {
  if (typeof name !== 'string' || name.length === 0) {
    return iconRegistry.portal
  }

  const normalizedName = name.replace(/^svg:/, '') as PortalIconName
  return iconRegistry[normalizedName] ?? iconRegistry.portal
}
