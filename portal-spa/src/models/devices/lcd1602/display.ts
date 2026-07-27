import { BaseDisplay, Mono1Render } from '../display/display.ts'

export class Lcd1602Display extends BaseDisplay<'mono1'> {
  readonly name = 'lcd1602'
  readonly defaultText = 'ABC'
  readonly defaultBitmapWidth = 1
  readonly defaultBitmapHeight = 1
  readonly maxBitmapBytes = 0
  readonly bitmapFormat = 'mono1' as const
  readonly render = new Mono1Render()
  readonly supportsColor = false
  readonly supportsBitmapImport = false
  readonly supportsAspectRatioLock = false
  readonly coordinateUnit = 'cell' as const
  readonly supportedWidgetTypes = ['character'] as const
  readonly supportedRotations = [0] as const
}

export const lcd1602Display = new Lcd1602Display()
