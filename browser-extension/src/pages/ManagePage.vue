<template>
  <div class="manage-screen flex flex-col h-full">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="manage-screen_content h-full flex flex-col items-start px-5">

      <h1 class="text-[var(--text-color)] text-[18px] mb-4">Settings</h1>

      <div class="w-full flex flex-col bg-[var(--section)] rounded-xl mb-5">
        <span
          v-for="(item, i) in LINKS"
          :key="i"
          @click="push(item.link)"
          class="cursor-pointer p-4 flex items-center justify-between items-center text-[15px] text-[var(--text-color)] hover:text-[var(--text-blue)]"
          :class="{ 'border-b-[1px] border-[var(--border-color)] dark:border-[#4e4e4e]': LINKS.length - 1 !== i }"
        >
          {{ item.label }}
          <ChevronIcon class="transform rotate-270 h-[15px]" />
        </span>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mt-auto">
        <Button
          second
          class="w-full px-0 flex items-center justify-center gap-2"
          @click="back"
        >
          Go Back
        </Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'

const LINKS = [
  {
    label: 'Manage Address',
    link: '/manage-address'
  },
  {
    label: 'Manage Password',
    link: '/manage-password'
  },
  {
    label: 'Manage Errors',
    link: '/manage-errors'
  }
]

const { back, push } = useRouter()
</script>

<style lang="scss" scoped>
.manage-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;

    p {
      font-size: 18px;
      line-height: 25px;
      margin-bottom: 15px;
    }
  }

  &_form span {
    text-align: left;
    font-weight: 400;
    font-size: 14px;
    line-height: 18px;
    letter-spacing: -0.154px;
  }

  &_info {
    font-weight: 500;
    font-size: 15px;
    line-height: 20px;
    display: flex;
    align-items: center;
    text-align: center;
    letter-spacing: -0.32px;
    color: #1b46f5;
  }
}
</style>
