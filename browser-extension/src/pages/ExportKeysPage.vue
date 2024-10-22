<template>
  <LoadingPage v-if="loading" />
  <div v-else class="start-screen flex flex-col h-full" data-testid="export-keys.page">
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
        <span>Export Key Pair...</span>
        <p class="text-[var(--text-grey-color)] mt-3 px-1">
          Incorrectly handling exported key pair can lead to breaches,
          unauthorized access, and data compromise.
        </p>
      </div>
      <!-- Buttons -->
      <div class="flex w-full gap-3 mt-auto">
        <Button @click="cancel" outline class="w-2/4" data-testid="cancel"> Cancel </Button>
        <Modal
          @on-submit="onExport"
          @on-close="password = ''"
          title="Unlock Your Wallet"
          class="w-2/4"
          text="Do you want to unload your keys?"
          submit-text="Yes"
          :disabled="isDisabledPass"
          submit-by-enter
        >
          <template #button="{ onClick }">
            <Button
              class="w-full"
              :disabled="isDisabled"
              style="white-space: nowrap"
              @click="onClick"
              data-testid="export"
              >Export</Button
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
                  data-testid="password"
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

import { ref, toRefs, computed, onMounted } from 'vue'
import WinHelpers from '~/helpers/winHelpers'
import { useStore } from '~/popup/store/index'
import { EXPORT_INSCRIPTION_KEY_PAIR } from '~/config/events'
import LoadingPage from '~/pages/LoadingPage.vue'
import CustomInput from '~/components/CustomInput.vue'
import Modal from '~/components/Modal.vue'
import { isASCII, sendMessage } from '~/helpers/index'
import { showSuccess } from '~/helpers'

const store = useStore()
const { dataForExportKeyPair } = toRefs(store)
const loading = ref(true)
const password = ref('')

const winHelpers = new WinHelpers()

const isDisabled = computed(() => {
  if (!isASCII(password.value)) return true
  return false
})

const isDisabledPass = computed(() => {
  if (isDisabled.value) return true
  if (!password.value?.length) return true
  return false
})

async function onExport() {
  const payload = {
    ...dataForExportKeyPair.value,
    password: password.value
  }
  const keyPairs = await sendMessage(
    EXPORT_INSCRIPTION_KEY_PAIR,
    payload,
    'background'
  )
  if (keyPairs) {
    const link = document.createElement('a')
    const file = new Blob([keyPairs?.privateKey], { type: 'text/plain' })
    link.href = URL.createObjectURL(file)
    link.download = 'key-pair.txt'
    link.click()
    URL.revokeObjectURL(link.href)
    link.remove()
    showSuccess('Success', 'File successfully downloaded')

    setTimeout(() => {
      cancel()
    }, 5000)
  }
}

function cancel() {
  winHelpers.closeCurrentWindow()
}

onMounted(() => {
  setTimeout(() => {
    loading.value = false
  }, 500)
})
</script>

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
