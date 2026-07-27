import { BaseDisplay, Mono1Render } from '../display/display.ts'

export class Tm1637Display extends BaseDisplay<'mono1'> {
  readonly name = 'tm1637'
  readonly defaultText = '1234'
  readonly defaultBitmapWidth = 1
  readonly defaultBitmapHeight = 1
  readonly maxBitmapBytes = 0
  readonly bitmapFormat = 'mono1' as const
  readonly render = new Mono1Render()
  readonly supportsColor = false
  readonly supportsBitmapImport = false
  readonly supportsAspectRatioLock = false
  readonly coordinateUnit = 'digit' as const
  readonly supportedWidgetTypes = ['digital'] as const
  readonly supportedRotations = [0, 180] as const

  override resolveDesignerRotation(rotation: number): number {
    return rotation === 180 ? 2 : 0
  }
}

export const tm1637Display = new Tm1637Display()
