<template>
  <svg
    xmlns="http://www.w3.org/2000/svg"
    class="text-black svg-icon"
    width="24"
    height="24"
    viewBox="0 0 24 24"
    stroke-width="2"
    stroke="currentColor"
    fill="none"
    stroke-linecap="round"
    stroke-linejoin="round"
    data-testid="copy-icon"
  >
    <path stroke="none" d="M0 0h24v24H0z" />
    <rect x="8" y="8" width="12" height="12" rx="2" />
    <path d="M16 8v-2a2 2 0 0 0 -2 -2h-8a2 2 0 0 0 -2 2v8a2 2 0 0 0 2 2h2" />
  </svg>
</template>

<script lang="ts">
import { defineComponent } from 'vue'

export default defineComponent({
  name: 'CopyIcon'
})
</script>

<style lang="scss" scoped>
.dark {
  .svg-icon {
    color: #fff;
  }
}
</style>
