<template>
  <component :is="tag" class="portal-icon-set" aria-hidden="true">
    <svg
      :viewBox="shape.viewBox"
      class="portal-icon-set__svg"
      xmlns="http://www.w3.org/2000/svg"
      fill="none"
      stroke="currentColor"
      stroke-linecap="round"
      stroke-linejoin="round"
      stroke-width="1.8"
    >
      <path v-for="path in shape.paths" :key="path" :d="path" />
    </svg>
  </component>
</template>

<script setup lang="ts">
import { computed } from 'vue'

import { iconRegistry, type AppIconName } from '@/icons'

type IconValue = string | undefined | null

const props = defineProps<{
  tag: string | object
  icon?: IconValue
}>()

const shape = computed(() => {
  if (typeof props.icon !== 'string') {
    return iconRegistry.portal
  }

  const name = props.icon.replace(/^svg:/, '') as AppIconName
  return iconRegistry[name] ?? iconRegistry.portal
})
</script>

<style scoped>
.portal-icon-set {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1em;
  height: 1em;
  line-height: 0;
  flex: none;
  font-size: inherit;
}

.portal-icon-set__svg {
  display: block;
  width: 1em;
  height: 1em;
  flex: none;
}
</style>
