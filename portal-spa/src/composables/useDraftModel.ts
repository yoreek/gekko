import type { DeviceRecord } from '@/api/contracts'

// The props shape shared by device Fields components (each SFC still declares its own
// defineProps so Vue can compile them; this type exists for call sites that need the shape).
export interface DeviceFieldsProps<Draft> {
  modelValue: Draft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}

// Shared v-model helper for device Fields components: emits a shallow copy of the draft with
// one field replaced. `props` must be the reactive object returned by defineProps -- reading
// `props.modelValue` at call time is what keeps the copy current.
export function useDraftModel<Draft extends object>(
  props: { modelValue: Draft },
  emit: (event: 'update:modelValue', value: Draft) => void,
) {
  function update<K extends keyof Draft>(key: K, value: Draft[K]): void {
    emit('update:modelValue', { ...props.modelValue, [key]: value })
  }
  return { update }
}
