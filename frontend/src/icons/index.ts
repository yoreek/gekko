export type AppIconName = 'portal' | 'wifi' | 'device' | 'ota' | 'system' | 'ws' | 'locale'

type IconShape = {
  viewBox: string
  paths: string[]
}

export const iconRegistry: Record<AppIconName, IconShape> = {
  portal: {
    viewBox: '0 0 24 24',
    paths: ['M4 6h16v12H4z', 'M7 9h10v2H7z', 'M7 13h6v2H7z'],
  },
  wifi: {
    viewBox: '0 0 24 24',
    paths: ['M12 18.5l2.5-2.5a3.5 3.5 0 00-5 0z', 'M6 12a9 9 0 0112 0', 'M3 8a14 14 0 0118 0'],
  },
  device: {
    viewBox: '0 0 24 24',
    paths: ['M5 5h14v14H5z', 'M9 9h6v6H9z'],
  },
  ota: {
    viewBox: '0 0 24 24',
    paths: ['M12 4l4 4h-3v7h-2V8H8z', 'M5 18h14v2H5z'],
  },
  system: {
    viewBox: '0 0 24 24',
    paths: ['M12 7a5 5 0 100 10 5 5 0 000-10z', 'M12 2v3M12 19v3M4.9 4.9l2.1 2.1M16.9 16.9l2.1 2.1M2 12h3M19 12h3M4.9 19.1l2.1-2.1M16.9 7.1l2.1-2.1'],
  },
  ws: {
    viewBox: '0 0 24 24',
    paths: ['M4 12a8 8 0 0116 0', 'M7 12a5 5 0 0110 0', 'M10 12a2 2 0 104 0'],
  },
  locale: {
    viewBox: '0 0 24 24',
    paths: ['M4 5h16v14H4z', 'M8 9h8M8 12h8M8 15h5'],
  },
}

