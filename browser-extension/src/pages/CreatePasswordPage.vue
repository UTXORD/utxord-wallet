<template>
  <div class="create-password-screen flex flex-col h-full" data-testid="create-password-page">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="create-password-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <!-- title and content -->
      <div class="w-full flex flex-col items-start mb-5">
        <p class="create-password-screen_title text-[var(--text-color)] mb-2">Create your Password</p>
        <span class="create-password-screen_description text-[var(--text-grey-color)]">
          Use it to protect and to unlock your wallet
        </span>
      </div>

      <!-- Inputs -->
      <div
        class="create-password-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl px-3 pt-3 mb-5"
      >
        <div class="create-password-screen_form-input flex flex-col mb-2">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Password</span
          >
          <CustomInput
            type="password"
            v-model="password"
            @change="upDatePasswordFromLocalStorage"
            autofocus
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters',
              (val) => isLength(val) || 'Password must be minimum 9 characters',
              (val) => isContains(val) || 'Password contains atleast One Uppercase, One Lowercase, One Number and One Special Chacter'
            ]"
            data-testid="password"
          />
        </div>
        <div class="create-password-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Confirm Password</span
          >
          <CustomInput
            type="password"
            v-model="confirmPassword"
            @change="upDatePasswordFromLocalStorage"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters',
              (val) =>
                val === password ||
                'Confirm Password does not match the Password',
              (val) => isLength(val) || 'Password must be minimum 9 characters',
              (val) => isContains(val) ||
                'Password contains atleast One Uppercase, One Lowercase, One Number and One Special Chacter'
            ]"
            data-testid="confirm-password"
          />
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mb-5 mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center"
          @click="goToBack"
          data-testid="go-back"
        >
          <ArrowLeftIcon />
        </Button>
        <Button @click="onConfirm" :disabled="isDisabled" enter class="w-full" data-testid="confirm">
          Confirm
        </Button>
      </div>

      <!-- Info -->
      <p class="create-password-screen_info text-[var(--text-blue)]">
        The data is stored locally in this extension
      </p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onBeforeMount } from 'vue'
import { useRouter } from 'vue-router'
import { SET_UP_PASSWORD } from '~/config/events'
import CustomInput from '~/components/CustomInput.vue'

import { isASCII, isLength, isContains, sendMessage } from '~/helpers/index'

const { back, push } = useRouter()

const password = ref('')
const confirmPassword = ref('')

const SET_UP_PASSWORD_PAGE = 'SET_UP_PASSWORD_PAGE'
const SET_UP_PASSWORD_CONFIRM_PAGE = 'SET_UP_PASSWORD_CONFIRM_PAGE'

function upDatePasswordFromLocalStorage(){
  console.log('password.value:', password.value)
  console.log('confirmPassword.value:', confirmPassword.value)
  localStorage?.setItem(SET_UP_PASSWORD_PAGE, password.value)
  localStorage?.setItem(SET_UP_PASSWORD_CONFIRM_PAGE, confirmPassword.value)
}

const isDisabled = computed(() => {
  if (!password.value?.length || !confirmPassword.value?.length) return true
  if (password.value !== confirmPassword.value) return true
  if (!isASCII(password.value) || !isASCII(confirmPassword.value)) return true
  if (!isLength(password.value) || !isLength(confirmPassword.value)) return true
  if (!isContains(password.value) || !isContains(confirmPassword.value)) return true
  return false
})

const page = computed(() =>{
  const current_page = window?.history?.state?.current?.split('#')[1] || localStorage?.getItem('current-page')
  if(!current_page) return 'start'
  return current_page;
})

function removeTempDataFromLocalStorage() {
  localStorage.removeItem(SET_UP_PASSWORD_PAGE)
  localStorage.removeItem(SET_UP_PASSWORD_CONFIRM_PAGE)
  localStorage.removeItem('current-page')
}

function goToBack() {
  removeTempDataFromLocalStorage()
  push('/start')
}

async function onConfirm() {
  try{
    const setuped = await sendMessage(
      SET_UP_PASSWORD, {
        password: password.value,
      },
      'background'
      )
    if (setuped) {
      localStorage?.setItem(SET_UP_PASSWORD, true)
      push(`/${page.value}`)
    }
  }catch(e){
    console.log('CreatePasswordPage->onConfirm():',e);
  }
}

async function getPassword() {
  const isPassSetUpd = Boolean(localStorage?.getItem(SET_UP_PASSWORD))
  if(isPassSetUpd === true){
  console.log('isPassSetUpd:', isPassSetUpd)
  console.log(`/${page.value}`)
    push(`/${page.value}`)
  }
  const tempPassword = localStorage?.getItem(SET_UP_PASSWORD_PAGE)
  const tempConfirmPassword = localStorage?.getItem(SET_UP_PASSWORD_CONFIRM_PAGE)

  console.log('tempPassword:', tempPassword);
  if (tempPassword) {
    password.value = tempPassword
    confirmPassword.value = tempConfirmPassword
  }
}

onBeforeMount(() => {
  console.log('onBeforeMount')
  const current_page = window?.history?.state?.current?.split('#')[1] || localStorage?.getItem('current-page')
  console.log('current_page:', current_page);
  if(current_page) localStorage?.setItem('current-page', current_page)
  getPassword()
})
</script>

<style lang="scss" scoped>
.create-password-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;
  }

  &_title {
    font-size: 18px;
    line-height: 25px;
  }

  &_description {
    font-size: 15px;
    line-height: 20px;
    font-weight: normal;
  }

  &_form span {
    text-align: left;
    font-weight: 400;
    font-size: 14px;
    line-height: 18px;
    letter-spacing: -0.154px;
  }

  &_info {
    font-size: 15px;
    line-height: 20px;
    display: flex;
    align-items: center;
    text-align: center;
    letter-spacing: -0.32px;
  }
  &_help {
    box-sizing: border-box;
    border-width: thin;
    border-style: solid;
    border-color: white;
    border-radius: 0.75rem;
    height: 10em;
  }
}
</style>
