import { BaseDisplay, Mono1Render } from '../display/display.ts'

export class Lcd2004Display extends BaseDisplay<'mono1'> {
  readonly name = 'lcd2004'
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
  readonly logicalWidth = 20
}

export const lcd2004Display = new Lcd2004Display()
