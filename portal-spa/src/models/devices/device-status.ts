export function deviceStatusLabelKey(status: string): string {
  switch (status.trim().toLowerCase()) {
    case 'ready':
      return 'device.status.ready'
    case 'disabled':
      return 'device.status.disabled'
    case 'dependency_blocked':
      return 'device.status.dependencyBlocked'
    case 'faulted':
      return 'device.status.faulted'
    case 'unknown':
      return 'device.status.unknown'
    default:
      return 'device.status.unknown'
  }
}
