<template>
  <LoadingPage v-if="loading" />
  <div v-else class="wallet-created-screen flex flex-col h-full" data-testid="wallet-created-page">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="wallet-created-screen_content h-full flex flex-col justify-center items-center px-5 pb-5">
      <div class="flex flex-col items-center mt-auto">
        <img
          class="wallet-created-screen_img mb-5"
          src="/assets/wallet-created.svg"
          alt="Success"
        />
        <h1>Wallet has been created successfully</h1>
        <div class="wallet-created-screen_text flex flex-col mt-3">
          Now you can use it
        </div>
      </div>
      <div class="flex w-full mt-auto">
        <Button class="w-full" enter @click="goToHomePage" data-testid="start">Start</Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
const { push } = useRouter()

const loading = ref(true)

function goToHomePage() {
  push('/')
}

onMounted(() => {
  setTimeout(() => {
    loading.value = false
  }, 500)
})
</script>

<style lang="scss" scoped>
.wallet-created-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;

    h1 {
      max-width: 280px;
      font-weight: 600;
      font-size: 22px;
      line-height: 30px;
      letter-spacing: 0.33px;
    }
  }

  &_text {
    font-weight: 400;
    font-size: 15px;
    line-height: 20px;
    letter-spacing: -0.01em;
    color: #AAABAD;
  }
}
</style>
