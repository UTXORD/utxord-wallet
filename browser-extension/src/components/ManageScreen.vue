<template>
  <div class="password-screen flex flex-col h-full">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="password-screen_content h-full flex flex-col items-start px-5">

      <p class="text-[var(--text-color)]">Manage account</p>
      <!-- Inputs -->
      <div class="password-screen_form-input flex flex-col p-3">
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >Use one-time addresses:
          <input
            name="useDerivation"
            type="checkbox"
            v-model="useDerivation"
            @change="setDerivate"
            />
        </span>
      </div>
      <p class="text-[var(--text-color)]">Manage password</p>
      <div
        class="password-screen_form w-full flex flex-col bg-[var(--section)] rounded-lg px-3 pt-3 mb-5"
      >
        <div class="password-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Old Password:</span
          >
          <CustomInput
            autofocus
            type="password"
            v-model="oldPassword"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters'
            ]"
          />
        </div>
        <div class="password-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >New Password:</span
          >
          <CustomInput
            type="password"
            v-model="password"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters'
            ]"
          />
        </div>
        <div class="password-screen_form-input flex flex-col">
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
      <div class="flex w-full mt-auto">
        <Button
          outline
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="back"
        >
          <img src="/assets/arrow-left.svg" alt="Go Back" />
        </Button>
        <Button :disabled="isDisabled" class="w-full" @click="onStore"
          >Save Password to Store</Button
        >
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { sendMessage } from 'webext-bridge'
import { computed, ref } from 'vue'
import { useRouter } from 'vue-router'
import { isASCII } from '~/helpers/index'
import { useStore } from '~/popup/store/index'

const store = useStore()
const { useDerivation, typeAddress } = toRefs(store)

const { back, push } = useRouter()

const oldPassword = ref('')
const password = ref('')
const confirmPassword = ref('')

const isDisabled = computed(() => {
  if (
    !password.value.length ||
    !confirmPassword.value.length ||
    !oldPassword.value.length
  )
    return true
  if (password.value !== confirmPassword.value) return true
  if (
    !isASCII(oldPassword.value) ||
    !isASCII(password.value) ||
    !isASCII(confirmPassword.value)
  )
    return true
  return false
})

async function setDerivate(){
  const res = await sendMessage(
    'CHANGE_USE_DERIVATION',
    {
      value: Boolean(useDerivation.value)
    },
    'background')
    store.setUseDerivation(Boolean(useDerivation.value))
    const ta = Number(!typeAddress.value);
    const tl = Boolean(useDerivation.value)?'fund':'oth'
    const addr = res.keys?.addresses?.reverse()?.find(
      (item) => item.type === tl && item.typeAddress === ta
    )?.address
    store.setFundAddress(addr)
}

async function onStore() {
  const saved = await sendMessage(
    'UPDATE_PASSWORD',
    {
      old: oldPassword.value,
      password: password.value
    },
    'background'
  )
  if (saved) {
    push('/')
  }
}
</script>

<style lang="scss" scoped>
.password-screen {
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
