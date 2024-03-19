<template>
  <div class="wallet-created-screen flex flex-col h-full">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="wallet-created-screen_content h-full flex flex-col items-start px-5">
      <p class="text-[var(--text-color)]">Wallet has been created successfully</p>
      <div class="wallet-created-screen_text flex flex-col">
        <span class="mb-2 w-full text-[var(--text-grey-color)]">
        Now you can use it
        </span>
      </div>
      <div class="flex w-full mt-auto">
        <Button class="w-full" @click="Start"
          >Start</Button
        >
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'
const { back, push } = useRouter()
async function Start() {
    push('/')
}
</script>

<style lang="scss" scoped>
.wallet-created-screen{
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;

    p {
      font-size: 18px;
      line-height: 25px;
      margin-bottom: 15px;
    }

  &_test span {
    text-align: left;
    font-weight: 400;
    font-size: 14px;
    line-height: 18px;
    letter-spacing: -0.154px;
  }
 }
}
</style>
