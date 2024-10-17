<template>
  <div class="password-screen flex flex-col h-full" data-testid="manage-password-page">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="password-screen_content h-full flex flex-col items-start px-5">

      <h1 class="text-[var(--text-color)] text-[18px] mb-4">Manage Password</h1>

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
            data-testid="old-password"
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
              (val) => isASCII(val) || 'Please enter only Latin characters',
              (val) => isLength(val) || 'Password must be minimum 9 characters',
              (val) => isContains(val) || 'Password contains atleast One Uppercase, One Lowercase, One Number and One Special Chacter'
            ]"
            data-testid="password"
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
      <div class="flex w-full mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center"
          @click="back"
          data-testid="go-back"
        >
          <ArrowLeftIcon />
        </Button>
        <Button :disabled="isDisabled" enter class="w-full" @click="onStore" data-testid="store">Store</Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useRouter } from 'vue-router'
import { isASCII, isLength, isContains, sendMessage } from '~/helpers/index'

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
  if (!isASCII(oldPassword.value))  return true
  if (!isASCII(password.value) ||  !isASCII(confirmPassword.value)) return true
  if (!isLength(password.value) || !isLength(confirmPassword.value)) return true
  if (!isContains(password.value) || !isContains(confirmPassword.value)) return true
  return false
})

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
}
</style>
