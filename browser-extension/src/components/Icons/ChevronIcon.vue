<template>
  <svg
    class="svg-icon"
    aria-hidden="true"
    fill="none"
    stroke="currentColor"
    viewBox="0 0 24 24"
    xmlns="http://www.w3.org/2000/svg"
    data-testid="chevron-icon"
  >
    <path
      stroke-linecap="round"
      stroke-linejoin="round"
      stroke-width="2"
      d="M19 9l-7 7-7-7"
      stroke="black"
    />
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
    > path {
      stroke: #fff;
    }
  }
}
</style>
