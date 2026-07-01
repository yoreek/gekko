<template>
  <v-container :fluid="dense" :class="dense ? 'pa-0' : 'pa-3 pa-sm-4 pa-md-6'">
    <slot />
  </v-container>
</template>

<script setup lang="ts">
withDefaults(defineProps<{
  /** Edge-to-edge layout (dashboard-style grids) instead of padded page content. */
  dense?: boolean
}>(), {
  dense: false,
})
</script>
