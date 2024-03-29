<template>
  <div class="dropdown relative w-full" v-click-outside="close">
    <button
      class="w-full min-w-[160px] min-h-[44px] text-[var(--text-color)] bg-[var(--bg-color)] text-left rounded-[10px] text-sm px-4 py-2.5 justify-between inline-flex items-center"
      type="button"
      :class="{ 'dropdown--opened': opened }"
      @click="toggle"
    >
      <span v-if="selected">{{ selected?.label || selected }}</span>
      <span v-else class="text-[var(--text-grey-color)] text-sm">Choose an option</span>
      <ChevronIcon class="h-3 ml-2" />
    </button>
    <!-- Dropdown menu -->
    <div
      v-if="options?.length && opened"
      class="w-full z-10 bg-[var(--bg-color)] divide-y divide-gray-100 rounded-[10px] shadow absolute top-12 left-0"
    >
      <ul
        class="p-1 text-sm text-left text-gray-700 dark:text-gray-200"
      >
        <li
          v-for="(option, i) in options"
          :key="i"
          class="px-[8px] py-[10px] cursor-pointer hover:bg-slate-100/10 rounded-lg"
          :class="{ 'dropdown-option--active font-semibold': selected?.value === option?.value }"
          @click="onSelect(option)"
        >
          {{ option?.label }}
        </li>
      </ul>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, PropType, ref } from 'vue'

interface IOption {
  label: string
  value: string | number
}

export default defineComponent({
  name: 'Dropdown',
  props: {
    options: {
      type: Array as PropType<IOption[]>,
      default: () => []
    },
    disabled: {
      type: Boolean,
      default: false
    },
    modelValue: {
      type: Object as PropType<IOption>,
      default: () => ({})
    }
  },
  emits: ['update:model-value'],
  setup(props, { emit }) {
    const selected = ref(props.modelValue)
    const opened = ref(false)

    function close() {
      opened.value = false
    }

    function onSelect(option: IOption) {
      selected.value = option;
      close();
      emit('update:model-value', option.value);
    }

    function toggle() {
      opened.value = !opened.value
    }

    return {
      selected,
      opened,
      close,
      toggle,
      onSelect,
    }
  }
})
</script>

<style lang="scss" scoped>
.dropdown {
  &--opened {
    outline: 1px solid rgba($color: var(--primary-rgb), $alpha: 0.5);
    box-shadow: 0 0 0 3px rgba($color: var(--primary-rgb), $alpha: 0.1);

    > svg {
      transform: rotate(180deg);
    }
  }
  &-option {
    &--active {
      background-color: rgba($color: var(--primary-rgb), $alpha: 0.16);
    }
  }
}
</style>