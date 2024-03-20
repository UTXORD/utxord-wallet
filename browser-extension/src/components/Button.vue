<template>
  <component
    :is="to ? 'router-link' : 'button'"
    :to="to"
    type="button"
    class="flex justify-center"
    :class="[{ 'cursor-not-allowed opacity-70': disabled }, classes]"
    @click="onClick"
  >
    <slot />
  </component>
</template>

<script setup lang="ts">
import { computed, defineEmits } from 'vue'

const props = defineProps({
  outline: {
    type: Boolean,
    default: false
  },
  disabled: {
    type: Boolean,
    default: false
  },
  second: {
    type: Boolean,
    default: false
  },
  to: {
    type: String,
    default: null
  }
})

const emit = defineEmits(['click'])

const classes = computed(() => {
  if (props.outline) {
    return 'text-gray-900 dark:text-white bg-[transparent] border border-black dark:border-white rounded-md text-[15px] px-5 py-[10px]'
  }
  if (props.second) {
    return 'text-white bg-black dark:bg-slate-100/10 active:bg-black-800 rounded-md text-[15px] px-5 py-[10px] text-center'
  }
  return 'text-white bg-black dark:bg-white dark:text-black active:bg-black-800 rounded-md text-[15px] px-5 py-[10px] text-center'
})

function onClick() {
  if (!props.disabled) {
    emit('click')
  }
}
</script>
