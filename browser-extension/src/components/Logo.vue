<template>
  <div class="logo p-4">
    <LogoIcon />
    <span class="network text-[var(--text-grey-color)]">{{ network }}</span>
  </div>
</template>
<script setup lang="ts">
import { ref, onMounted } from 'vue'
import useWallet from '~/popup/modules/useWallet'
import LogoIcon from '~/components/Icons/LogoIcon.vue'

const { getNetWork } = useWallet()
const network = ref('')
onMounted(() => {
  setTimeout(async () => {
    network.value = await getNetWork()
  }, 10)
})
</script>
<style scoped>
.network {
  float: left;
}
.logo img {
  width: 126px;
}
</style>
