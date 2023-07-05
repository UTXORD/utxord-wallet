<template>
  <div :style="{ fontSize }" class="price-comp text-black">
    {{ resultPrice }} Sat
  </div>
</template>

<script setup lang="ts">
import { computed, defineProps, ref, watch, toRefs } from 'vue'

const props = defineProps({
  price: {
    type: Number,
    default: 0
  },
  fontSizeBreakpoints: {
    type: Object,
    default: () => ({})
  }
})

const { price, fontSizeBreakpoints } = toRefs(props)

const fontSize = ref('16px')

const USDollar = new Intl.NumberFormat('en-US', {
  style: 'decimal',
  currency: 'USD'
})

const resultPrice = computed(() => USDollar.format(price.value || 0))

function findClosestNumber(target: number, numbers: number[]) {
  return numbers.reduce(function (prev, curr) {
    return Math.abs(curr - target) < Math.abs(prev - target) ? curr : prev
  })
}

function getFontSize(p: number): string {
  const point = findClosestNumber(
    p,
    Object.keys(fontSizeBreakpoints.value).map((k) => +k)
  )
  return fontSizeBreakpoints.value[point]
}

watch(
  price,
  () => {
    fontSize.value = getFontSize(price.value)
  },
  { immediate: true }
)
</script>

<style lang="scss" scoped>
.price-comp {
  font-weight: 600;
}
</style>
