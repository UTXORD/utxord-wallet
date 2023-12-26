<template>
  <main class="w-full h-full text-center text-gray-700 dark:text-gray-200">
    <router-view />
    <NotifySuccess />
    <NotifyError />
  </main>
</template>

<script setup lang="ts">
import { sendMessage, onMessage } from 'webext-bridge'
import { onBeforeMount } from 'vue'
import { useRouter } from 'vue-router'
import NotifySuccess from '~/components/NotifySuccess.vue'
import NotifyError from '~/components/NotifyError.vue'
import { useStore } from '~/popup/store/index'
import {
  CHECK_AUTH,
  EXCEPTION,
  WARNING,
  NOTIFICATION,
  SAVE_DATA_FOR_SIGN,
  SAVE_DATA_FOR_EXPORT_KEY_PAIR
} from '~/config/events'
import useWallet from '~/popup/modules/useWallet'
import { showError } from '~/helpers'

const { push } = useRouter()
const {
  getFundAddress,
  getOrdAddress,
  getBalance,
  saveDataForSign,
  saveDataForExportKeyPair
} = useWallet()
const store = useStore()

function redirectByQuery() {
  const pageHref = window.location.search
  const searchParams = new URLSearchParams(
    pageHref.substring(pageHref.indexOf('?'))
  )
  const page = searchParams.get('page')
  if (page) {
    push(page)
  }
}

async function checkAuth(): Promise<boolean> {
  try {
    const success = await sendMessage(CHECK_AUTH, {}, 'background')
    if (!success) {
      return false
    } else {
      return true
    }
  } catch (error) {
    showError(EXCEPTION, error.message)
    console.log(error)
  }
}

async function updateBalance() {
  const address = await getFundAddress()
  await getOrdAddress()
  getBalance(address)
  setInterval(async () => {
    store.setRefreshingBalance()
    setTimeout(() => {
      getBalance(address)
      store.unsetRefreshingBalance()
    }, 1000)
  }, 5000)
}

async function init() {
  const success = await checkAuth()
  if (success) {
    redirectByQuery()
    updateBalance()
  } else {
    const tempMnemonic = localStorage?.getItem('temp-mnemonic')
    if (tempMnemonic) {
      push('/generate')
    } else {
      push('/start')
    }
  }
}



// We have to use chrome API instead of webext-bridge module due to following issue
// https://github.com/zikaari/webext-bridge/issues/37
let port = chrome.runtime.connect({
  name: 'POPUP_MESSAGING_CHANNEL'
});
port.postMessage({id: 'POPUP_MESSAGING_CHANNEL_OPEN'});
port.onMessage.addListener(async function(payload) {
  switch (payload.id) {
    case DO_REFRESH_BALANCE: {
      store.setBalance({
        ...balance.value,
        connect: payload.connect
      });
      refreshBalance();
      break;
    }
    case BALANCE_REFRESH_DONE: {
      const fresh_balance = payload.data?.balance
      store.setBalance({
        ...fresh_balance || balance.value,
        sync: true,
        connect: true
      });
      break;
    }
    case UPDATE_PLUGIN_CONNECT: {
      const justConnected = !balance?.value?.connect && payload.connect;
      store.setBalance({
        ...balance.value,
        sync: justConnected ? false : balance?.value?.sync,
        connect: payload.connect
      });
      break;
    }
  }
});

onMessage(EXCEPTION, (payload: any) => {
  showError(EXCEPTION, payload?.data)
  return true
})

onMessage(WARNING, (payload: any) => {
  showError(WARNING, payload?.data)
  return true
})

onMessage(NOTIFICATION, (payload: any) => {
  showSuccess(NOTIFICATION, payload?.data, 10000)
  return true
})

onMessage(SAVE_DATA_FOR_SIGN, (payload) => {
  saveDataForSign(payload.data)
  return true
})

onMessage(SAVE_DATA_FOR_EXPORT_KEY_PAIR, (payload) => {
  saveDataForExportKeyPair(payload.data)
  return true
})

onBeforeMount(() => {
  init()
})
</script>
