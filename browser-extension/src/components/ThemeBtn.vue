<template>
  <q-btn class="theme-btn cursor-pointer" dense outline @click="toggle" data-testid="theme-btn">
    <component class="theme-btn_icon" :is="comp" />
  </q-btn>
</template>

<script setup>
import { computed } from 'vue'
import SunIcon from '~/components/Icons/SunIcon.vue'
import MoonIcon from '~/components/Icons/MoonIcon.vue'
import { isDark, mode } from '~/popup/modules/useTheme'

const comp = computed(() => {
  if (isDark.value) return SunIcon
  return MoonIcon
})

function toggle() {
  mode.value = isDark.value ? 'light' : 'dark'
  if (localStorage) {
    localStorage?.setItem('dark-theme', isDark)
  }
}
</script>

<style lang="scss" scoped>
.theme-btn {
  &_icon {
    transform: rotate(200deg);
  }
}
</style>
