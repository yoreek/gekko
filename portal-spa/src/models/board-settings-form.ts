import type { BoardSettingsResponse } from '@/api'

export interface BoardSettingsSnapshot {
  settings: BoardSettingsResponse
}

export interface BoardSettingsDraft {
  selectedBoardId: string
}

export function createBoardSettingsDraft(source: BoardSettingsSnapshot): BoardSettingsDraft {
  return {
    selectedBoardId: source.settings.selectedBoardId,
  }
}

export function isBoardSettingsDirty(
  source: BoardSettingsSnapshot,
  draft: BoardSettingsDraft,
): boolean {
  return source.settings.selectedBoardId !== draft.selectedBoardId
}
