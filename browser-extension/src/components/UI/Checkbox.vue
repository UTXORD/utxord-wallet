<template>
  <div
    class="checkbox flex items-center gap-2 cursor-pointer"
    :class="{ 'checkbox--checked': selected }"
    @click="toggle"
  >
    <div class="checkbox_figure bg-slate-100/1 flex items-center justify-center rounded border-1 border-[#AAABAD] w-[24px] min-w-[24px] h-[24px]">
      <CheckIcon />
    </div>
    <slot />
  </div>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue'

export default defineComponent({
  name: 'Checkbox',
  props: {
    modelValue: {
      type: Boolean,
      default: false
    }
  },
  emits: ['update:model-value'],
  setup(props, { emit }) {
    const selected = ref(props.modelValue)

    function toggle() {
      selected.value = !selected.value;
      emit('update:model-value', selected.value);
    }

    return {
      selected,
      toggle
    }
  }
})
</script>

<style lang="scss" scoped>
.checkbox {
  &_figure {
    background: rgba($color: #fff, $alpha: 0.1);

    svg {
      display: none;
    }
  }

  &--checked {
    :deep(.checkbox_figure) {
      background: var(--primary);
      border-color: var(--primary);

      svg {
        display: block;

        path {
          fill: #000 !important;
        }
      }
    }
  }
}
</style>
