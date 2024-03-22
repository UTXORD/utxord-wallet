<template>
  <div class="password-screen flex flex-col h-full">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="password-screen_content h-full flex flex-col items-start px-5">

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
          class="w-full px-0 flex items-center justify-center bg-white gap-2"
          @click="back"
        >
          Go Back
        </Button>
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

const LINKS = [
  {
    label: 'Manage Address',
    link: '/manage-address'
  },
  {
    label: 'Manage Password',
    link: '/manage-password'
  }
]

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

async function showManagePassword() {
  push('/manage-password');
}

async function showManageAddress() {
  push('/manage-address');
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
