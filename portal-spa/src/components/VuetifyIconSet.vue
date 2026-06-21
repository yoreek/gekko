<template>
  <component :is="tag" class="portal-icon-set" aria-hidden="true">
    <svg
      :viewBox="shape.viewBox"
      class="portal-icon-set__svg"
      xmlns="http://www.w3.org/2000/svg"
      :fill="shape.filled ? 'currentColor' : 'none'"
      :stroke="shape.filled ? 'none' : 'currentColor'"
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

import { resolveIconShape } from '@/icons'

type IconValue = string | undefined | null

const props = defineProps<{
  tag: string | object
  icon?: IconValue
}>()

const shape = computed(() => {
  return resolveIconShape(props.icon)
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
