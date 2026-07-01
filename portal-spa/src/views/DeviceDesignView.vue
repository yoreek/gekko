<template>
  <div v-if="designerComponent" class="device-designer-page">
    <component
      :is="designerComponent"
      :model-value="draft"
      :device="device"
      mode="edit"
      :busy="loading"
      @update:model-value="(v: any) => { draft.value = v }"
      @command="(submitCommand as any)"
      @close="navigateBack"
    />
  </div>
  <div v-else class="d-flex align-center justify-center min-h-screen">
    <v-progress-circular indeterminate />
  </div>
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

const { device, loading, draft, submitCommand } = detail

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
</script>

<style scoped>
.device-designer-page {
  width: 100%;
  height: 100%;
}

.min-h-screen {
  min-height: 100vh;
}
</style>
