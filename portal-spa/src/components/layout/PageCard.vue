<template>
  <v-card>
    <v-card-item>
      <slot name="header" />
    </v-card-item>

    <v-divider />

    <v-card-text>
      <slot />
    </v-card-text>

    <v-card-actions v-if="$slots.actions">
      <slot name="actions" />
    </v-card-actions>
  </v-card>
</template>

<script setup lang="ts">
defineProps<{}>()
</script>
