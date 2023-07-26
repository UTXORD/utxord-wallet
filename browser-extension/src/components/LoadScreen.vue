<template>
  <div class="load-screen flex flex-col h-full">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="load-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <!-- Secret phrase -->
      <div
        class="load-screen_form w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
      >
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >Paste your secret 12 words here:</span
        >
        <CustomInput
          type="textarea"
          class="w-full"
          autofocus
          rows="3"
          v-model="textarea"
        />
      </div>

      <!-- Inputs -->
      <div
        class="load-screen_form w-full flex flex-col bg-[var(--section)] rounded-lg px-3 pt-3 mb-5"
      >
        <div class="load-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Enter your Password:</span
          >
          <CustomInput
            type="password"
            v-model="password"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters'
            ]"
          />
        </div>
        <div class="load-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Confirm Password:</span
          >
          <CustomInput
            type="password"
            v-model="confirmPassword"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters'
            ]"
          />
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mb-5 mt-auto">
        <Button
          outline
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="back"
        >
          <img src="/assets/arrow-left.svg" alt="Go Back" />
        </Button>
        <Button @click="onStore" :disabled="isDisabled" class="w-full"
          >Load</Button
        >
      </div>

      <!-- Info -->
      <p class="load-screen_info text-[var(--text-blue)]">
        The data is stored locally in this extension
      </p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { sendMessage } from 'webext-bridge'
import { useRouter } from 'vue-router'
import { SAVE_GENERATED_SEED } from '~/config/events'
import useWallet from '~/popup/modules/useWallet'
import { isASCII } from '~/helpers/index'

const { back, push } = useRouter()
const { getFundAddress, getBalance } = useWallet()
const textarea = ref('')
const password = ref('')
const confirmPassword = ref('')

const isDisabled = computed(() => {
  const seedArr = textarea.value.trim().split(' ')
  if (seedArr.length !== 12) return true
  if (!textarea.value) return true
  if (!password.value.length || !confirmPassword.value.length) return true
  if (password.value !== confirmPassword.value) return true
  if (!isASCII(password.value) || !isASCII(confirmPassword.value)) return true
  return false
})

async function onStore() {
  const generated = await sendMessage(
    SAVE_GENERATED_SEED,
    {
      seed: textarea.value,
      password: password.value
    },
    'background'
  )
  if (generated) {
    await getFundAddress()
    getBalance()
    push('/')
  }
}
</script>

<style lang="scss" scoped>
.load-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;
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
  }
}
</style>
