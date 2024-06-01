<template>
  <svg v-if="opened" width="14" height="12" viewBox="0 0 14 12" fill="none" xmlns="http://www.w3.org/2000/svg" class="svg-icon" data-testid="eye-icon">
    <path fill-rule="evenodd" clip-rule="evenodd" d="M9.10767 6.03476C9.10767 7.19876 8.16367 8.14209 6.99967 8.14209C5.83567 8.14209 4.89233 7.19876 4.89233 6.03476C4.89233 4.87009 5.83567 3.92676 6.99967 3.92676C8.16367 3.92676 9.10767 4.87009 9.10767 6.03476Z" stroke="black" stroke-linecap="round" stroke-linejoin="round"/>
    <path fill-rule="evenodd" clip-rule="evenodd" d="M6.99865 10.903C9.53731 10.903 11.8593 9.07766 13.1666 6.03499C11.8593 2.99233 9.53731 1.16699 6.99865 1.16699H7.00131C4.46265 1.16699 2.14065 2.99233 0.833313 6.03499C2.14065 9.07766 4.46265 10.903 7.00131 10.903H6.99865Z" stroke="black" stroke-linecap="round" stroke-linejoin="round"/>
  </svg>
  <svg v-else width="14" height="12" viewBox="0 0 14 12" fill="none" xmlns="http://www.w3.org/2000/svg" class="svg-icon" data-testid="eye-icon">
  <path d="M5.50705 7.57811C5.12371 7.19545 4.89038 6.67545 4.89038 6.09211C4.89038 4.92345 5.83171 3.98145 6.99971 3.98145C7.57771 3.98145 8.10971 4.21545 8.48638 4.59811" stroke="black" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="M9.06987 6.46582C8.91521 7.32582 8.23787 8.00449 7.37854 8.16049" stroke="black" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="M3.4364 9.64799C2.3784 8.81732 1.4824 7.60399 0.833069 6.09132C1.48907 4.57199 2.39107 3.35199 3.45574 2.51466C4.51374 1.67732 5.7344 1.22266 6.99974 1.22266C8.2724 1.22266 9.4924 1.68399 10.5571 2.52732" stroke="black" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="M11.965 3.99414C12.4237 4.60347 12.827 5.30681 13.1664 6.09147C11.855 9.12947 9.53771 10.9595 6.99971 10.9595C6.42437 10.9595 5.85704 10.8661 5.31171 10.6841" stroke="black" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="M12.258 0.833008L1.742 11.349" stroke="black" stroke-linecap="round" stroke-linejoin="round"/>
  </svg>
</template>

<script lang="ts">
import { defineComponent } from 'vue'

export default defineComponent({
  name: 'EyeIcon',
  props: {
    opened: {
      type: Boolean,
      default: false,
    }
  }
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
