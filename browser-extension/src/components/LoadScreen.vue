<template>
  <div class="load-screen flex flex-col h-full">
    <Logo />
    <div
      class="load-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <!-- Secret phrase -->
      <div
        class="load-screen_form w-full flex flex-col bg-white rounded-lg p-3 mb-5"
      >
        <span class="mb-2 w-full">Paste your secret 12 words here:</span>
        <textarea
          v-model="textarea"
          autofocus
          rows="3"
          class="w-full"
        ></textarea>
      </div>

      <!-- Inputs -->
      <div
        class="load-screen_form w-full flex flex-col bg-white rounded-lg px-3 pt-3 mb-5"
      >
        <div class="load-screen_form-input flex flex-col">
          <span class="mb-2 w-full">Enter your Password:</span>
          <CustomInput
            type="password"
            v-model="password"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters'
            ]"
          />
        </div>
        <div class="load-screen_form-input flex flex-col">
          <span class="mb-2 w-full">Confirm Password:</span>
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
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-[#EDEDED]"
          @click="back"
        >
          <img src="/assets/arrow-left.svg" alt="Go Back" />
        </Button>
        <Button @click="onStore" :disabled="isDisabled" class="w-full"
          >Load</Button
        >
      </div>

      <!-- Info -->
      <p class="load-screen_info">
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
    border-top: 1px solid #e8e8e8;
  }

  &_form textarea {
    background: #f2f3f5;
    border: 0.5px solid rgba(0, 0, 0, 0.12);
    border-radius: 4px;
    font-weight: 400;
    font-size: 15px;
    line-height: 20px;
    display: flex;
    align-items: center;
    letter-spacing: -0.32px;
    color: #000000;
    padding: 12px;
  }

  &_form span {
    text-align: left;
    font-weight: 400;
    font-size: 14px;
    line-height: 18px;
    letter-spacing: -0.154px;
    color: #6d7885;
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
