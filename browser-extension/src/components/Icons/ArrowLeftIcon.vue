<template>
  <svg width="14" height="12" viewBox="0 0 14 12" fill="none" xmlns="http://www.w3.org/2000/svg" class="svg-icon" data-testid="arrow-left-icon">
    <path d="M13.6669 6.6394L13.6669 5.30607L2.67664 5.30607L6.56513 1.60522L5.64589 0.639404L0.540548 5.49837C0.264689 5.76094 0.264445 6.20085 0.540019 6.4637L5.64537 11.3335L6.56565 10.3687L2.65597 6.6394L13.6669 6.6394Z" fill="white"/>
  </svg>
</template>

<script lang="ts">
import { defineComponent } from 'vue'

export default defineComponent({
  name: 'ArrowLeftIcon'
})
</script>

<style lang="scss" scoped>
.dark {
  .svg-icon {
    > path {
      fill: #fff;
    }
  }
}
</style>
