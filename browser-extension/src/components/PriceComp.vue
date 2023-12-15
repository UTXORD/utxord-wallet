<template>
  <div
    :style="{ fontSize }"
    class="price-comp text-[var(--text-color)] flex items-center gap-[5px]"
  >
    <div class="flex items-center gap-[5px]">
      <LoaderIcon
        v-if="loading"
        :class="`loader animate-spin w-${loadingSize} h-${loadingSize}`"
      />
      <template v-else> {{ resultPrice }} </template>
      Sat
    </div>
    <span class="opacity-50 text-xs">
      (~${{ resultUSD }})
    </span>
  </div>
</template>

<script setup lang="ts">
import { computed, defineProps, ref, watch, toRefs } from 'vue'
import { convertSatsToUSD } from '~/helpers'
import useWallet from '~/popup/modules/useWallet'

import LoaderIcon from '~/components/Icons/LoaderIcon.vue'

const props = defineProps({
  price: {
    type: Number,
    default: 0
  },
  loading: {
    type: Boolean,
    default: false
  },
  loadingSize: {
    type: Number,
    default: 4
  },
  fontSizeBreakpoints: {
    type: Object,
    default: () => ({})
  }
})

const { price, fontSizeBreakpoints } = toRefs(props)
const { usd } = useWallet();

const fontSize = ref('16px')

const USDollar = new Intl.NumberFormat('en-US', {
  style: 'decimal',
  currency: 'USD'
})

const resultPrice = computed(() => USDollar.format(price.value || 0))
const resultUSD = computed(() => convertSatsToUSD(price.value || 0, usd.value).toFixed(2))

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
