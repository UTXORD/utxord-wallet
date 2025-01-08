<template>
  <main class="w-full h-full text-center text-gray-700 dark:text-gray-200">
    <router-view />
    <NotifySuccess />
    <NotifyError />
  </main>
</template>

<script setup lang="ts">
import browser from 'webextension-polyfill'
import { sendMessage } from '~/helpers/messenger'
import { onMessage } from 'webext-bridge'
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
  SAVE_DATA_FOR_EXPORT_KEY_PAIR,
  POPUP_HEARTBEAT,
  DO_REFRESH_BALANCE,
  BALANCE_REFRESH_DONE,
  UPDATE_PLUGIN_CONNECT
} from '~/config/events'
import useWallet from '~/popup/modules/useWallet'
import { showError, showSuccess } from '~/helpers'
import { toRefs } from 'vue'

const { push } = useRouter()
const {
  getFundAddress,
  getOrdAddress,
  getBalance,
  saveDataForSign,
  saveDataForExportKeyPair,
  fetchUSDRate
} = useWallet()

const store = useStore()
const { balance, fundAddress } = toRefs(store)

function redirectByQuery() {
  return setTimeout(()=>{
  /**
    * Important: Make the transition to the signature page the last one in the timeout,
    * otherwise the page restore will work and you will lose the signature page
  */
    const pageHref = window.location.search
    const searchParams = new URLSearchParams(
      pageHref.substring(pageHref.indexOf('?'))
    )
    const page = searchParams.get('page')
    if (page) {
      return push(page);
    }
  },1);

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
    console.log('checkAuth()->message:',error.message)
  }
  return false;
}

function runHeartbeat() {
  setInterval(async () => {
    await sendMessage(POPUP_HEARTBEAT, {}, 'background')
  }, 6000)
}


function refreshBalance() {
  store.setSyncToFalse();
  setTimeout(async () => {
    await getBalance(fundAddress.value)
  }, 500)
}

async function init() {
  const success = await checkAuth()
  if (success) {
    redirectByQuery()
    runHeartbeat()
    // refreshBalance()
    fetchUSDRate()
  }
}



// We have to use browser API instead of webext-bridge module due to following issue
// https://github.com/zikaari/webext-bridge/issues/37
let port = browser.runtime.connect({
  name: 'POPUP_MESSAGING_CHANNEL'
});
port.postMessage({id: 'POPUP_MESSAGING_CHANNEL_OPEN'});
port.onMessage.addListener(async function(payload) {
  switch (payload?.id) {
    case DO_REFRESH_BALANCE: {
      store.setBalance({
        ...balance.value,
        connect: payload?.connect
      });
      refreshBalance();
      break;
    }
    case BALANCE_REFRESH_DONE: {
      const fresh_balance = payload.data
      store.setBalance({
        ...fresh_balance || balance.value,
        sync: true,
        connect: true
      });
      break;
    }
    case UPDATE_PLUGIN_CONNECT: {
      const justConnected = !balance?.value?.connect && payload?.connect;
      store.setBalance({
        ...balance.value,
        sync: justConnected ? false : balance?.value?.sync,
        connect: payload?.connect
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
  saveDataForSign(payload.data || {})
  return true
})

onMessage(SAVE_DATA_FOR_EXPORT_KEY_PAIR, (payload) => {
  saveDataForExportKeyPair(payload.data || {})
  return true
})

onBeforeMount(() => {
  // console.info('===== onBeforeMount')
  init()
})
</script>
