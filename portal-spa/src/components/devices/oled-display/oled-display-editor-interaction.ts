import type { OledDisplayWidget } from '@/models/devices/oled-display-layout'

export interface OledDisplayCanvasInteraction {
  mode: 'drag' | 'resize'
  widgetId: string
  startClientX: number
  startClientY: number
  startX: number
  startY: number
  startWidth: number
  startHeight: number
}

export function resolveOledDisplayInteractionWidgets(
  widgets: OledDisplayWidget[],
  interaction: OledDisplayCanvasInteraction,
  currentClientX: number,
  currentClientY: number,
  zoom: number,
  deviceWidth: number,
  deviceHeight: number,
): OledDisplayWidget[] {
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
