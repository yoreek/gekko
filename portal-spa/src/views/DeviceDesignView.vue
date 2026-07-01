<template>
  <component
    :is="designerComponent"
    v-if="designerComponent"
    model-value
    :device="(device as any)"
    @update:model-value="(v: boolean) => { if (!v) navigateBack() }"
    @save="onSave"
  />
</template>

<script setup lang="ts">
import { computed, inject } from 'vue'
import { useRouter } from 'vue-router'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import type { UseDeviceDetailReturn } from '@/composables/useDeviceDetail'

const props = defineProps<{
  id: number
}>()

const router = useRouter()
const detail = inject<UseDeviceDetailReturn>('deviceDetail')

if (!detail) {
  throw new Error('DeviceDesignView requires deviceDetail to be provided')
}

const { device, save } = detail

const designerComponent = computed(() => {
  const dev = device.value as any
  if (!dev) return null
  const typeName = dev.record?.typeName
  if (!typeName) return null
  const ui = resolveDeviceUi(typeName)
  return ui.designerComponent || null
})

function navigateBack(): void {
  router.push({ name: 'device-detail', params: { id: props.id } })
}

async function onSave(payload: Record<string, unknown>): Promise<void> {
  await save(payload as any)
  navigateBack()
}
</script>
