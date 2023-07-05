<template>
  <main class="w-full h-full text-center text-gray-700 dark:text-gray-200">
    <router-view />
    <NotifySuccess />
    <NotifyError />
  </main>
</template>

<script setup lang="ts">
import { sendMessage, onMessage } from 'webext-bridge'
import { onBeforeMount, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import NotifySuccess from '~/components/NotifySuccess.vue'
import NotifyError from '~/components/NotifyError.vue'
import { CHECK_AUTH, EXCEPTION, SAVE_DATA_FOR_SIGN } from '~/config/events'
import useWallet from '~/popup/modules/useWallet'
import { showError } from '~/helpers'

const { push } = useRouter()
const { getFundAddress, getOrdAddress, getBalance, saveDataForSign } =
  useWallet()

async function checkAuth(): Promise<boolean> {
  try{
    const success = await sendMessage(CHECK_AUTH, {}, 'background')
    if (!success) {
      push('/start')
      return false
    } else {
      return true
    }
  }catch(error){
    showError(EXCEPTION, error.message)
    console.log(error);
  }
}

function redirectTo() {
  const pageHref = window.location.search
  const searchParams = new URLSearchParams(
    pageHref.substring(pageHref.indexOf('?'))
  )
  const page = searchParams.get('page')
  if (page) {
    push(page)
  }
}

async function init() {
  const success = await checkAuth()
  if (success) {
    const address = await getFundAddress()
    await getOrdAddress()
    getBalance(address)
  }
}

onMessage(EXCEPTION, (payload: any) => {
  showError(EXCEPTION, payload?.data)
  return true
})

onMessage(SAVE_DATA_FOR_SIGN, (payload) => {
  saveDataForSign(payload.data)
  return true
})

onBeforeMount(() => {
  redirectTo()
})

onMounted(() => {
  init()
})
</script>
