<template>
  <PageContainer dense>
    <PageCard>
      <template #header>
        <PageToolbar
          :title="t('device.dialog.ssd1306Display.designerTitle')"
          :subtitle="device?.config?.name || ''"
          show-back
          @back="navigateBack"
        />
      </template>

      <v-alert type="info" variant="tonal" density="comfortable">
        Display designer - simplified view (placeholder)
      </v-alert>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onBeforeMount } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageCard from '@/v2/components/layout/PageCard.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'

const props = defineProps<{
  id: number
}>()

const { t } = useI18n()
const router = useRouter()
const deviceStore = useDeviceRegistryStore()

const device = computed(() => deviceStore.devices.find(d => d.record.id === props.id))

onBeforeMount(async () => {
  await deviceStore.initialize()
})

function navigateBack(): void {
  router.back()
}
</script>
