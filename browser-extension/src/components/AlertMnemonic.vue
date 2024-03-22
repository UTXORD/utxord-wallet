<template>
  <div class="alert-mnemonic-screen flex flex-col h-full">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="alert-mnemonic-screen_content h-full flex flex-col items-start px-5">
      <h1 class="text-[var(--text-color)]">Back up your mnemonic phrase</h1>

      <div class="alert-mnemonic-screen_text flex flex-col">
        <p class="mb-2 w-full text-left text-[var(--text-grey-color)]">
          You will be shown a mnemonic phrase on the next screen. Write it down and keep in a safe place
        </p>
        <p class="w-full text-left text-[var(--text-grey-color)]">
          This phrase is the <span class="text-[var(--text-color)]">only</span> way to recover your wallet
        </p>
      </div>
      <div class="flex w-full mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="back"
        >
          <ArrowLeftIcon />
        </Button>
        <Button
          enter
          class="w-full"
          @click="GotIt"
        >Got it</Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'
const { back, push } = useRouter()
async function GotIt() {
  push('/generate')
}
</script>

<style lang="scss" scoped>
.alert-mnemonic-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;

    h1 {
      font-size: 18px;
      line-height: 25px;
      margin-bottom: 15px;
    }
  }

  &_text {
    p {
      font-weight: 400;
      font-size: 15px;
      line-height: 20px;
      letter-spacing: -0.32px;

      span {
        font-weight: 600;
      }
    }
  }
}
</style>
