<template>
  <div class="generate-screen h-full flex flex-col">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="generate-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <!-- Secret phrase -->
      <div
        class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
      >
        <div class="flex items-center mb-2">
          <span class="w-full text-[var(--text-grey-color)]"
            >Store these safely:</span
          >
          <RefreshIconSeed @click="refreshMnemonic" class="cursor-pointer w-4 mr-2" />
          <CopyIcon
            class="cursor-pointer"
            @click="copyToClipboard(textarea, 'Mnemonic was copied!')"
          />
        </div>
        <CustomInput
          type="textarea"
          class="w-full"
          rows="3"
          v-model="textarea"
          readonly
        />
      </div>

      <!-- Inputs -->
      <div
        class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl px-3 pt-3 mb-5"
      >
        <div class="generate-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Enter your Password:</span
          >
          <CustomInput
            autofocus
            type="password"
            v-model="password"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters'
            ]"
          />
        </div>
        <div class="generate-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Confirm Password:</span
          >
          <CustomInput
            type="password"
            v-model="confirmPassword"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters',
              (val) =>
                val === password ||
                'Confirm Password does not match the Password'
            ]"
          />
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mb-5 mt-auto">
        <Button
          outline
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="goToStartPage"
        >
          <img src="/assets/arrow-left.svg" alt="Go Back" />
        </Button>
        <Button :disabled="isDisabled" class="w-full" @click="onStore"
          >Store</Button
        >
      </div>

      <!-- Info -->
      <p class="generate-screen_info text-[var(--text-blue)]">
        The data is stored locally in this extension
      </p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { sendMessage } from 'webext-bridge'
import { computed, ref, onBeforeMount } from 'vue'
import { useRouter } from 'vue-router'
import useWallet from '~/popup/modules/useWallet'
import { SAVE_GENERATED_SEED } from '~/config/events'
import { isASCII, copyToClipboard } from '~/helpers/index'

const { push } = useRouter()
const { getFundAddress, getBalance } = useWallet()
const textarea = ref('')
const password = ref('')
const confirmPassword = ref('')

const isDisabled = computed(() => {
  if (!textarea.value) return true
  if (!password.value.length || !confirmPassword.value.length) return true
  if (password.value !== confirmPassword.value) return true
  if (!isASCII(password.value) || !isASCII(confirmPassword.value)) return true
  return false
})

async function onStore() {
  const success = await sendMessage(
    SAVE_GENERATED_SEED,
    {
      seed: textarea.value,
      password: password.value
    },
    'background'
  )
  if (success) {
    const fundAddress = await getFundAddress()
    getBalance(fundAddress)
    localStorage.removeItem('temp-mnemonic')
    push('/')
  }
}

function goToStartPage() {
  localStorage.removeItem('temp-mnemonic')
  push('/start')
}

function refreshMnemonic() {
  localStorage.removeItem('temp-mnemonic')
  getMnemonic()
}

async function getMnemonic() {
  const tempMnemonic = localStorage?.getItem('temp-mnemonic')
  if (tempMnemonic) {
    textarea.value = tempMnemonic
  } else {
    const mnemonic = await sendMessage('GENERATE_MNEMONIC', {}, 'background')
    localStorage?.setItem('temp-mnemonic', mnemonic)
    textarea.value = mnemonic
  }
}

onBeforeMount(() => {
  getMnemonic()
})
</script>

<style lang="scss" scoped>
.generate-screen {
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
