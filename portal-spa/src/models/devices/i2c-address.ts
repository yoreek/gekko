export function formatI2cAddress(value: number): string {
  return value.toString(16).toUpperCase().padStart(2, '0')
}

export function parseI2cAddress(value: string | number): number {
  const text = String(value).trim().replace(/^0x/i, '')
  if (!/^[0-9a-fA-F]{1,2}$/.test(text)) {
    return Number.NaN
  }
  return Number.parseInt(text, 16)
}
