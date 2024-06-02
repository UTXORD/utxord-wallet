<template>
  <div v-show="isExistMessage"
      class="sign-screen_block w-full flex items-center bg-[var(--section)] rounded-lg p-3 mb-5"
      data-testid="notify-in-body"
  >
    <span class="mr-2 text-[var(--text-color)]" style="font-size: 20px;">{{ message }}</span>

  </div>
</template>
<script setup lang="ts">
import { toRefs } from 'vue'
import { useStore } from '~/popup/store/index'

const store = useStore()
const { dataForSign } = toRefs(store)

const message = computed(() => {
 return dataForSign.value?.data?.costs?.errorMessage || dataForSign.value?.data?.errorMessage
})

const isExistMessage = computed(() => {
  if (message?.value?.length) return true
  return false
})

</script>

<style scoped>
.sign-screen_block span {
  text-align: left;
  font-weight: 400;
  font-size: 14px;
  line-height: 18px;
  letter-spacing: -0.154px;
}
</style>
