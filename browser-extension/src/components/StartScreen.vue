<template>
  <div class="start-screen flex flex-col h-full">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
        class="start-screen_content h-full flex flex-col justify-between px-5 pb-5"
    >
      <div class="start-screen_lock-key flex flex-col">
        <img
            class="start-screen_lock-key_img mb-5"
            src="/assets/lockKey.svg"
            alt="Lock"
        />
        <span>Let’s load a mnemonic phrase...</span>
      </div>
      <div class="start-screen_form bg-[var(--section)] rounded-lg p-4">
        <div class="flex items-center">
          <span class="mr-5 text-left text-[var(--text-color)]"
          >I want a new one</span
          >
          <Button to="/generate" class="ml-auto w-[100px]">Generate</Button>
        </div>
        <div
            class="w-full min-h-[1px] bg-[var(--border-color)] my-4 dark:bg-[#555555]"
        />
        <div class="flex items-center">
          <span class="mr-5 text-left text-[var(--text-color)]"
          >I have it</span
          >
          <Button to="/load" class="ml-auto w-[100px]" outline>Load</Button>
        </div>
      </div>
    </div>
  </div>
</template>

<style lang="scss" scoped>
.start-screen {
  &_lock-key {
    padding-top: 100px;
    padding-bottom: 100px;
  }

  &_lock-key_img {
    height: 73px;
    width: auto;
  }

  &_lock-key span {
    font-weight: 600;
    font-size: 20px;
    line-height: 27px;
  }

  &_form span {
    font-weight: 400;
    font-size: 15px;
    line-height: 20px;
    letter-spacing: -0.01em;
  }
}
</style>
<script setup lang="ts">
</script>
