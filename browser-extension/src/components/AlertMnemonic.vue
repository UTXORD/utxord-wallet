<template>
  <div class="alert-mnemonic-screen flex flex-col h-full">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="alert-mnemonic-screen_content h-full flex flex-col items-start px-5">
      <p class="text-[var(--text-color)]">Back up your mnemonic phrase</p>

      <div class="alert-mnemonic-screen_text flex flex-col">
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >You will be shown a mnemonic phrase on the next screen. Write it down and keep in a safe place
          This phrase is the only way to recover your wallet
        </span>
      </div>
      <div class="flex w-full mt-auto">
        <Button
          outline
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="back"
        >
          <img src="/assets/arrow-left.svg" alt="Go Back" />
        </Button>
        <Button class="w-full" @click="GotIt"
          >Got it</Button
        >
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
.alert-mnemonic-screen{
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
