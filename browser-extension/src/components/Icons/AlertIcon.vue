<template>
  <svg width="16" height="16" viewBox="0 0 16 16" fill="none" xmlns="http://www.w3.org/2000/svg" class="svg-icon" data-testid="alert-icon">
    <path d="M12.6693 13.4005H3.3305C2.30797 13.4005 1.66881 12.2938 2.17975 11.4079L6.8786 3.26452C7.39147 2.37539 8.67567 2.37863 9.184 3.27035L13.8239 11.4137C14.3284 12.2996 13.6886 13.4005 12.6693 13.4005Z" stroke="black" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="M8.0078 8.85512V6.84766M8.00653 10.8853V10.8529" stroke="black" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
  </svg>
</template>

<script lang="ts">
import { defineComponent } from 'vue'

export default defineComponent({
  name: 'AlertIcon'
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
