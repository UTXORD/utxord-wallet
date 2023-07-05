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
  to: {
    type: String,
    default: null
  }
})

const emit = defineEmits(['click'])

const classes = computed(() => {
  if (props.outline) {
    return 'text-gray-900 bg-[transparent] border border-black font-medium rounded-md text-sm px-5 py-2.5'
  }
  return 'text-white bg-black active:bg-black-800 font-medium rounded-md text-sm px-5 py-2.5 text-center'
})

function onClick() {
  if (!props.disabled) {
    emit('click')
  }
}
</script>
