export type AppIconName =
  | 'portal'
  | 'wifi'
  | 'device'
  | 'devices'
  | 'panels'
  | 'ota'
  | 'system'
  | 'ws'
  | 'locale'
  | 'sun'
  | 'moon'
  | 'refresh'
  | 'close'
  | 'edit'
  | 'info'
  | 'trash'
  | 'menu'
  | 'plus'
  | 'power'
  | 'chevron-left'
  | 'chevron-right'
  | 'chevron-down'

type IconShape = {
  viewBox: string
  paths: string[]
}

export const iconRegistry: Record<AppIconName, IconShape> = {
  portal: {
    viewBox: '0 0 24 24',
    paths: ['M4 5h16v14H4z', 'M8 9h3v3H8z', 'M13 9h3v3h-3z', 'M8 14h8'],
  },
  wifi: {
    viewBox: '0 0 24 24',
    paths: ['M4 9a12 12 0 0116 0', 'M7.5 12.5a7 7 0 019 0', 'M10.5 16a3 3 0 013 0', 'M12 19h.01'],
  },
  device: {
    viewBox: '0 0 24 24',
    paths: ['M7 4h10v16H7z', 'M10 7h4', 'M10 17h4', 'M9 10h6v4H9z'],
  },
  devices: {
    viewBox: '0 0 24 24',
    paths: ['M4 5h6v6H4z', 'M14 5h6v6h-6z', 'M4 13h6v6H4z', 'M14 13h6v6h-6z', 'M10 8h4', 'M10 16h4'],
  },
  panels: {
    viewBox: '0 0 24 24',
    paths: ['M4 5h5v14H4z', 'M11 5h9v5h-9z', 'M11 12h9v7h-9z'],
  },
  ota: {
    viewBox: '0 0 24 24',
    paths: ['M7 16h10a4 4 0 00.5-7.97A6 6 0 006.3 9.6 3.5 3.5 0 007 16z', 'M12 16V8', 'M9 11l3-3 3 3'],
  },
  system: {
    viewBox: '0 0 24 24',
    paths: ['M12 8a4 4 0 100 8 4 4 0 000-8z', 'M12 3v3', 'M12 18v3', 'M4.8 6.2l2.1 2.1', 'M17.1 15.7l2.1 2.1', 'M3 12h3', 'M18 12h3', 'M4.8 17.8l2.1-2.1', 'M17.1 8.3l2.1-2.1'],
  },
  ws: {
    viewBox: '0 0 24 24',
    paths: ['M6 8h12', 'M6 16h12', 'M8 6a2 2 0 110 4 2 2 0 010-4z', 'M16 14a2 2 0 110 4 2 2 0 010-4z', 'M16 6a2 2 0 110 4 2 2 0 010-4z', 'M8 14a2 2 0 110 4 2 2 0 010-4z'],
  },
  locale: {
    viewBox: '0 0 24 24',
    paths: ['M12 4a8 8 0 100 16 8 8 0 000-16z', 'M4 12h16', 'M12 4a12 12 0 010 16', 'M12 4a12 12 0 000 16'],
  },
  sun: {
    viewBox: '0 0 24 24',
    paths: ['M12 8a4 4 0 100 8 4 4 0 000-8z', 'M12 3v2', 'M12 19v2', 'M5 12H3', 'M21 12h-2', 'M6.3 6.3L4.9 4.9', 'M19.1 19.1l-1.4-1.4', 'M6.3 17.7l-1.4 1.4', 'M19.1 4.9l-1.4 1.4'],
  },
  moon: {
    viewBox: '0 0 24 24',
    paths: ['M19 15.4A7.8 7.8 0 018.6 5a8 8 0 1010.4 10.4z'],
  },
  refresh: {
    viewBox: '0 0 24 24',
    paths: ['M20 6v5h-5', 'M19.2 11a7.5 7.5 0 00-12.9-4.7', 'M4 18v-5h5', 'M4.8 13a7.5 7.5 0 0012.9 4.7'],
  },
  close: {
    viewBox: '0 0 24 24',
    paths: ['M7 7l10 10', 'M17 7L7 17'],
  },
  edit: {
    viewBox: '0 0 24 24',
    paths: ['M5 19l4.2-1 9.3-9.3-3.2-3.2L6 14.8z', 'M13.8 7l3.2 3.2', 'M5 19h14'],
  },
  info: {
    viewBox: '0 0 24 24',
    paths: ['M12 17v-6', 'M12 8h.01', 'M12 21a9 9 0 100-18 9 9 0 000 18z'],
  },
  trash: {
    viewBox: '0 0 24 24',
    paths: ['M4 7h16', 'M9 7V4h6v3', 'M7 7l1 13h8l1-13', 'M10 11v6', 'M14 11v6'],
  },
  menu: {
    viewBox: '0 0 24 24',
    paths: ['M5 7h14', 'M5 12h14', 'M5 17h14'],
  },
  plus: {
    viewBox: '0 0 24 24',
    paths: ['M12 5v14', 'M5 12h14'],
  },
  power: {
    viewBox: '0 0 24 24',
    paths: ['M12 3v8', 'M7.1 6.8a7 7 0 109.8 0'],
  },
  'chevron-left': {
    viewBox: '0 0 24 24',
    paths: ['M15 6l-6 6 6 6'],
  },
  'chevron-right': {
    viewBox: '0 0 24 24',
    paths: ['M9 6l6 6-6 6'],
  },
  'chevron-down': {
    viewBox: '0 0 24 24',
    paths: ['M6 9l6 6 6-6'],
  },
}
