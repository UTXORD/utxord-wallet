<template>
  <div
    class="alert-box w-full text-left flex items-center gap-2 px-[10px] py-[8px] rounded-md mt-1"
    :class="{ 'alert-box--danger': danger }"
    data-testid="alert-box"
  >
    <AlertIcon class="alert-box-icon" />
    <div class="alert-box-slot">
      <slot />
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue'

export default defineComponent({
  name: 'AlertBox',
  props: {
    danger: {
      type: Boolean,
      default: true,
    }
  }
})
</script>

<style lang="scss" scoped>
.alert-box {
  &--danger {
    background: rgba(255, 86, 86, 0.16);
  }

  :deep(.alert-box-icon) {
    path {
      stroke: rgba(255, 86, 86, 1) !important;
    }
  }
  :deep(.alert-box-slot) {
    * {
      color: rgba(255, 86, 86, 1) !important;
    }
  }
}
</style>
