<template>
  <div
    :style="{ fontSize }"
    class="text-comp text-[var(--text-color)] flex items-center gap-[5px]"
  >
    <div class="flex items-center gap-[5px]">
      <LoaderIcon
        v-if="loading"
        :class="`loader animate-spin w-${loadingSize} h-${loadingSize}`"
      />
      <template v-else> {{ resultText }} </template>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, defineProps, ref, watch, toRefs } from 'vue'

import LoaderIcon from '~/components/Icons/LoaderIcon.vue'

const props = defineProps({
  text: {
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

const { text, fontSizeBreakpoints } = toRefs(props)

const fontSize = ref('16px')

const resultText = computed(() => text.value || '')

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
  text,
  () => {
    fontSize.value = getFontSize(text.value)
  },
  { immediate: true }
)
</script>

<style lang="scss" scoped>
.text-comp {
  font-weight: 600;
}
</style>
