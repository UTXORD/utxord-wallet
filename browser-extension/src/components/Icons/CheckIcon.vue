<template>
  <svg width="14" height="10" viewBox="0 0 14 10" fill="none" xmlns="http://www.w3.org/2000/svg" class="svg-icon">
    <path d="M13.7071 1.70711C14.0976 1.31658 14.0976 0.683417 13.7071 0.292893C13.3166 -0.0976311 12.6834 -0.0976311 12.2929 0.292893L5 7.58579L1.70711 4.29289C1.31658 3.90237 0.683417 3.90237 0.292893 4.29289C-0.0976311 4.68342 -0.0976311 5.31658 0.292893 5.70711L4.29289 9.70711C4.68342 10.0976 5.31658 10.0976 5.70711 9.70711L13.7071 1.70711Z" fill="black"/>
  </svg>
</template>

<script lang="ts">
import { defineComponent } from 'vue'

export default defineComponent({
  name: 'CheckIcon'
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
