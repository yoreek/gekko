import type { Ssd1306Widget } from '@/models/devices/ssd1306/layout'

export interface Ssd1306CanvasInteraction {
  mode: 'drag' | 'resize'
  widgetId: string
  keepAspectRatio: boolean
  startClientX: number
  startClientY: number
  startX: number
  startY: number
  startWidth: number
  startHeight: number
  startAspectRatio: number
}

export function resolveSsd1306InteractionWidgets(
  widgets: Ssd1306Widget[],
  interaction: Ssd1306CanvasInteraction,
  currentClientX: number,
  currentClientY: number,
  zoom: number,
  deviceWidth: number,
  deviceHeight: number,
): Ssd1306Widget[] {
  const scale = Math.max(1, Number.isFinite(zoom) ? zoom : 1)
  const deltaX = Math.round((currentClientX - interaction.startClientX) / scale)
  const deltaY = Math.round((currentClientY - interaction.startClientY) / scale)
  return widgets.map(widget => {
    if (widget.id !== interaction.widgetId) {
      return widget
    }
    if (interaction.mode === 'drag') {
      return {
        ...widget,
        x: clamp(interaction.startX + deltaX, 0, Math.max(0, deviceWidth - widget.width)),
        y: clamp(interaction.startY + deltaY, 0, Math.max(0, deviceHeight - widget.height)),
      }
    }
    if (interaction.keepAspectRatio) {
      const aspectRatio = interaction.startAspectRatio > 0 ? interaction.startAspectRatio : 1
      const preferredWidth = clamp(interaction.startWidth + deltaX, 1, Math.max(1, deviceWidth - widget.x))
      const preferredHeight = clamp(Math.round(preferredWidth / aspectRatio), 1, Math.max(1, deviceHeight - widget.y))
      const adjustedWidth = clamp(Math.round(preferredHeight * aspectRatio), 1, Math.max(1, deviceWidth - widget.x))
      return {
        ...widget,
        width: adjustedWidth,
        height: preferredHeight,
      }
    }
    return {
      ...widget,
      width: clamp(interaction.startWidth + deltaX, 1, Math.max(1, deviceWidth - widget.x)),
      height: clamp(interaction.startHeight + deltaY, 1, Math.max(1, deviceHeight - widget.y)),
    }
  })
}

function clamp(value: number, min: number, max: number): number {
  return Math.max(min, Math.min(max, value))
}
