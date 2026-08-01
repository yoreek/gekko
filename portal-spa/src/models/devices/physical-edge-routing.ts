export interface Point {
  x: number
  y: number
}

export interface Rect {
  x: number
  y: number
  width: number
  height: number
}

export interface PhysicalEdgeRouteInput {
  source: Point
  target: Point
  sourceSide: 'left' | 'right'
  controllerBody: Rect
  controllerBounds?: Rect
  gutter?: number
  laneOffset?: number
  borderRadius?: number
}

export interface PhysicalEdgeRoute {
  points: Point[]
  path: string
  label: Point
}

function moveTowards(from: Point, to: Point, distance: number): Point {
  if (from.x === to.x) {
    return { x: from.x, y: from.y + Math.sign(to.y - from.y) * distance }
  }
  return { x: from.x + Math.sign(to.x - from.x) * distance, y: from.y }
}

function roundedOrthogonalPath(points: readonly Point[], borderRadius: number): string {
  if (points.length === 0) return ''
  if (points.length === 1) return `M ${points[0].x} ${points[0].y}`

  const commands = [`M ${points[0].x} ${points[0].y}`]
  for (let index = 1; index < points.length - 1; index += 1) {
    const previous = points[index - 1]
    const corner = points[index]
    const next = points[index + 1]
    const incomingLength = Math.abs(corner.x - previous.x) + Math.abs(corner.y - previous.y)
    const outgoingLength = Math.abs(next.x - corner.x) + Math.abs(next.y - corner.y)
    const radius = Math.min(borderRadius, incomingLength / 2, outgoingLength / 2)
    const beforeCorner = moveTowards(corner, previous, radius)
    const afterCorner = moveTowards(corner, next, radius)
    commands.push(`L ${beforeCorner.x} ${beforeCorner.y}`)
    commands.push(`Q ${corner.x} ${corner.y} ${afterCorner.x} ${afterCorner.y}`)
  }
  const last = points[points.length - 1]
  commands.push(`L ${last.x} ${last.y}`)
  return commands.join(' ')
}

export function routePhysicalEdge(input: PhysicalEdgeRouteInput): PhysicalEdgeRoute {
  const gutter = input.gutter ?? 24
  const laneDistance = gutter + (input.laneOffset ?? 0)
  const bounds = input.controllerBounds ?? input.controllerBody
  const boundsLeft = bounds.x
  const boundsRight = bounds.x + bounds.width
  const boundsTop = bounds.y
  const boundsBottom = bounds.y + bounds.height
  const sourceGutterX = input.sourceSide === 'left' ? input.source.x - laneDistance : input.source.x + laneDistance
  const targetSide = input.target.x < (boundsLeft + boundsRight) / 2 ? 'left' : 'right'
  const targetGutterX = targetSide === 'left' ? boundsLeft - laneDistance : boundsRight + laneDistance
  const sameOutsideSide = input.sourceSide === targetSide

  let points: Point[]
  if (sameOutsideSide) {
    points = [
      input.source,
      { x: sourceGutterX, y: input.source.y },
      { x: sourceGutterX, y: input.target.y },
      input.target,
    ]
  } else {
    const topCost = Math.abs(input.source.y - (boundsTop - laneDistance)) + Math.abs(input.target.y - (boundsTop - laneDistance))
    const bottomCost = Math.abs(input.source.y - (boundsBottom + laneDistance)) + Math.abs(input.target.y - (boundsBottom + laneDistance))
    const routeY = topCost <= bottomCost ? boundsTop - laneDistance : boundsBottom + laneDistance
    points = [
      input.source,
      { x: sourceGutterX, y: input.source.y },
      { x: sourceGutterX, y: routeY },
      { x: targetGutterX, y: routeY },
      { x: targetGutterX, y: input.target.y },
      input.target,
    ]
  }

  const compact = points.filter((point, index) => index === 0 || point.x !== points[index - 1].x || point.y !== points[index - 1].y)
  const labelIndex = Math.floor((compact.length - 1) / 2)
  const a = compact[labelIndex]
  const b = compact[labelIndex + 1] ?? a
  return {
    points: compact,
    path: roundedOrthogonalPath(compact, input.borderRadius ?? 5),
    label: { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 },
  }
}
