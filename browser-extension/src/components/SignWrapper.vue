<template>
  <LoadingScreen v-if="loading" />
  <div v-else class="sign-screen h-full flex flex-col">
    <Header />
    <Logo />
    <div class="w-full h-[1px] bg-[var(--border-color)]" />
    <div
      class="sign-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <slot />

      <!-- Buttons -->
      <div class="flex w-full gap-3 mt-auto">
        <Button @click="cancel" outline class="w-2/4"> {{ !isInsufficientBalance?'Cancel':'Ok' }} </Button>
        <Modal
          @on-submit="onSign"
          @on-close="password = ''"
          title="Unlock Your Wallet"
          class="w-2/4"
          text="Do you want to unload your keys?"
          submit-text="Yes"
          :disabled="isDisabledPass"
          submit-by-enter
        >
          <template #button="{ onClick }" v-if="!isInsufficientBalance">
            <Button
              class="w-full"
              :disabled="isDisabled"
              style="white-space: nowrap"
              @click="onClick"
              >{{ isDisabledMessage }}</Button
            >
          </template>
          <template #body>
            <!-- Inputs -->
            <div
              class="password-screen_form w-full flex flex-col bg-[var(--section)] rounded-lg px-3 pt-3"
            >
              <div class="password-screen_form-input flex text-left flex-col">
                <span class="mb-2 w-full text-[var(--text-grey-color)]"
                  >Password</span
                >
                <CustomInput
                  autofocus
                  type="password"
                  v-model="password"
                  :rules="[
                    (val) =>
                      isASCII(val) || 'Please enter only Latin characters'
                  ]"
                />
              </div>
            </div>
          </template>
        </Modal>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { sendMessage } from 'webext-bridge'
import { ref, toRefs, computed, onMounted } from 'vue'
import WinHelpers from '~/helpers/winHelpers'
import { useStore } from '~/popup/store/index'
import {
  SELL_INSCRIPTION,
  CREATE_INSCRIPTION,
  SUBMIT_SIGN,
  BALANCE_CHANGE_PRESUMED,
  CREATE_CHUNK_INSCRIPTION
} from '~/config/events'
import LoadingScreen from '~/components/LoadingScreen.vue'
import CustomInput from '~/components/CustomInput.vue'
import Modal from '~/components/Modal.vue'
import { isASCII } from '~/helpers/index'

const store = useStore()
const { balance, dataForSign } = toRefs(store)
const loading = ref(true)
const password = ref('')

const winHelpers = new WinHelpers()

const total = computed(() => {
  let out = 0
  if (dataForSign.value?.type === CREATE_INSCRIPTION || dataForSign.value?.type === CREATE_CHUNK_INSCRIPTION) {
    out += dataForSign.value?.data?.costs?.amount || 0
  } else {
    out += dataForSign.value?.data?.ord_price || 0
    out += dataForSign.value?.data?.market_fee || 0
    out += dataForSign.value?.data?.expect_amount || 0
    out += dataForSign.value?.data?.market_fee || 0
  }
  return out
})

const isInsufficientBalance = computed(() => {
  if (Number(total.value) > Number(balance.value?.confirmed)) return true
  return false
})

const message = computed(() => {
 return dataForSign.value?.data?.costs?.errorMessage || dataForSign.value?.data?.errorMessage
})

const isDisabled = computed(() => {
  if (dataForSign.value?.type === SELL_INSCRIPTION) return false
  if (Number(balance.value?.confirmed) === 0) return true
  if (isInsufficientBalance.value) return true
  if (message?.value?.length) return true
  if (!isASCII(password.value)) return true
  return false
})

const isDisabledMessage = computed(() => {
  if (dataForSign.value?.type === SELL_INSCRIPTION) return 'Sign'
  if (Number(balance.value?.confirmed) === 0) return 'No Balance'
  if (isInsufficientBalance.value) return 'Insufficient Balance'
  return 'Sign'
})

const isDisabledPass = computed(() => {
  if (isDisabled.value) return true
  if (!password.value.length) return true
  return false
})

async function onSign() {
  dataForSign.value = { ...dataForSign.value, password: password.value }
  await sendMessage(BALANCE_CHANGE_PRESUMED, {}, 'background')
  await sendMessage(SUBMIT_SIGN, dataForSign.value, 'background')
}

function cancel() {
  winHelpers.closeCurrentWindow()
}

onMounted(() => {
  setTimeout(() => {
    loading.value = false
  }, 1000)
})
</script>

<style scoped>
.sign-screen_content {
  padding-top: 22px;
  padding-bottom: 22px;
}
</style>
