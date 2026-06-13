<template>
  <svg
    :viewBox="shape.viewBox"
    class="app-icon"
    xmlns="http://www.w3.org/2000/svg"
    fill="none"
    stroke="currentColor"
    stroke-linecap="round"
    stroke-linejoin="round"
    stroke-width="1.8"
    aria-hidden="true"
  >
    <path v-for="path in shape.paths" :key="path" :d="path" />
  </svg>
</template>

<script setup lang="ts">
import { computed } from 'vue'

import { iconRegistry, type AppIconName } from '@/icons'

const props = defineProps<{
  name: AppIconName
}>()

const shape = computed(() => iconRegistry[props.name] ?? iconRegistry.portal)
</script>

<style scoped>
.app-icon {
  display: block;
  width: 20px;
  height: 20px;
  flex: none;
}
</style>
