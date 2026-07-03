<template>
  <v-dialog :model-value="modelValue" max-width="520" @update:model-value="$emit('update:modelValue', $event)">
    <v-card v-if="device">
      <DeviceIdentityHeader :device="device" :dense="false">
        <template #prepend>
          <v-icon :icon="ui.icon" />
        </template>
        <template #append>
          <v-btn icon="edit" variant="text" size="small" :aria-label="t('device.dialog.edit')" @click="goToEdit" />
          <v-btn icon="close" variant="text" size="small" :aria-label="t('actions.close')" @click="$emit('update:modelValue', false)" />
        </template>
      </DeviceIdentityHeader>

      <v-card-text class="pt-0">
        <component :is="ui.widgetComponent" :device="device" :dense="false" @command="$emit('command', $event)" />
      </v-card-text>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'

import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import { resolveDeviceUiV2 } from '@/v2/components/registry/device-ui-registry'
import DeviceIdentityHeader from '@/v2/components/devices/common/DeviceIdentityHeader.vue'

const props = defineProps<{
  device: DeviceRecord | null
  modelValue: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const router = useRouter()

const ui = computed(() => resolveDeviceUiV2(props.device?.record.typeName))

function goToEdit(): void {
  if (!props.device) {
    return
  }
  const routeName = ui.value.designerComponent ? 'v2-device-design' : 'v2-device-detail'
  emit('update:modelValue', false)
  void router.push({ name: routeName, params: { id: props.device.record.id } })
}
</script>
