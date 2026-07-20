// Presets purely pre-fill numeric fields in the NTC create/edit form -- they are never persisted
// (the firmware config only stores the resulting numbers), so choosing one and then editing a
// field afterwards is always safe and never "fights" the preset.
export interface NtcPreset {
  id: string
  labelKey: string
  seriesResistorOhms: number
  nominalResistanceOhms: number
  betaCoefficient: number
}

export const ntcPresets: NtcPreset[] = [
  {
    id: 'generic10k3950',
    labelKey: 'device.dialog.ntcThermistor.presets.generic10k3950',
    seriesResistorOhms: 10000,
    nominalResistanceOhms: 10000,
    betaCoefficient: 3950,
  },
  {
    id: 'epcos10k3435',
    labelKey: 'device.dialog.ntcThermistor.presets.epcos10k3435',
    seriesResistorOhms: 10000,
    nominalResistanceOhms: 10000,
    betaCoefficient: 3435,
  },
  {
    id: 'vishay10k3977',
    labelKey: 'device.dialog.ntcThermistor.presets.vishay10k3977',
    seriesResistorOhms: 10000,
    nominalResistanceOhms: 10000,
    betaCoefficient: 3977,
  },
  {
    id: 'semitec100k4267',
    labelKey: 'device.dialog.ntcThermistor.presets.semitec100k4267',
    seriesResistorOhms: 100000,
    nominalResistanceOhms: 100000,
    betaCoefficient: 4267,
  },
]

export const ntcCustomPresetId = 'custom'
