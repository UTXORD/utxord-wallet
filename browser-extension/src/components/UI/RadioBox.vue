<template>
  <label
    class="radio flex items-center gap-2 cursor-pointer"
    :class="{ 'radio--checked': checked }"
  >
    <input class="hidden" type="radio" :value="value" :checked="checked" @change="set" />
    <slot name="left" />
    <div class="radio_figure bg-slate-100/1 flex items-center justify-center rounded-full border-1 border-[#AAABAD] w-[16px] min-w-[16px] h-[16px]">
      <div v-if="checked" class="radio_figure-dot bg-[var(--primary)] rounded-full w-[8px] min-w-[8px] h-[8px]" />
    </div>
    <slot />
  </label>
</template>

<script lang="ts">
/**
 * @usage:
 *
 *   <RadioBox value="foo" v-model="selectedValue">Label 1</RadioBox>
 *   <RadioBox value="bar" v-model="selectedValue">Label 2</RadioBox>
 *   <RadioBox value="baz" v-model="selectedValue">Label 3</RadioBox>
 * 
 * setup() {
 *    return {
 *      selectedValue: ref('foo'),
 *    }
 *  }
 */

import { defineComponent, computed } from 'vue'

export default defineComponent({
  name: 'RadioBox',
  props: {
    modelValue: {
      type: String || Number || Boolean,
      required: true
    },
    value: {
      type: String || Number || Boolean,
      required: true
    }
  },
  emits: ['update:model-value'],
  setup(props, { emit }) {
    const checked = computed(() => props.modelValue === props.value)

    function set() {
      emit('update:model-value', props.value);
    }

    return {
      checked,
      set
    }
  }
})
</script>

<style lang="scss" scoped>
.radio {
  &_figure {
    background: rgba($color: #fff, $alpha: 0.1);

    svg {
      display: none;
    }
  }

  &--checked {
    :deep(.radio_figure) {
      border-color: var(--primary);
      box-shadow: 0 0 0 3px rgba($color: var(--primary-rgb), $alpha: 0.1);
    }
  }
}
</style>
