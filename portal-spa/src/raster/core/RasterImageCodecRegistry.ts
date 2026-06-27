import type { RasterImageFormat } from '../raster-image-types.ts'
import { Gray8RasterImageCodec } from '../gray8/Gray8RasterImageCodec.ts'
import { Mono1RasterImageCodec } from '../mono1/Mono1RasterImageCodec.ts'
import { Rgb565RasterImageCodec } from '../rgb565/Rgb565RasterImageCodec.ts'
import type { RasterImageCodec } from './RasterImageCodec.ts'

export class RasterImageCodecRegistry {
  private readonly codecs: Record<RasterImageFormat, RasterImageCodec> = {
    mono1: new Mono1RasterImageCodec(),
    gray8: new Gray8RasterImageCodec(),
    rgb565: new Rgb565RasterImageCodec(),
  }

  get(format: RasterImageFormat): RasterImageCodec {
    return this.codecs[format]
  }
}

export const rasterImageCodecRegistry = new RasterImageCodecRegistry()
