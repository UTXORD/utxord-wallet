<template>
  <div class="logo p-4">
    <img src="/assets/logo.svg" alt="UTXORD Wallet" />
    <span class="network text-gray-600">{{ network }}</span>
  </div>

</template>
<script setup lang="ts">
import { ref, toRefs, computed, onMounted } from 'vue'
import useWallet from '~/popup/modules/useWallet'
const { getNetWork } = useWallet()
const network = ref('')
onMounted(() => {
  setTimeout( async () => {
    network.value = await getNetWork()
  }, 10)
})
</script>
<style scoped>
.network{
  float: left;
}
.logo img {
  width: 126px;
}
</style>
