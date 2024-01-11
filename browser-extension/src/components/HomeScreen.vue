<template>
  <div class="home-screen flex flex-col h-full">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="home-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <!-- Balance -->
      <div
        class="home-screen_block relative w-full flex flex-col items-center bg-[var(--section)] rounded-lg px-3 py-5 mb-4"
      >
        <template v-if="connected">
          <Button
            outline
            :disabled="!isSynchronized"
            class="home-screen_balance-refresh absolute top-2 right-2"
            @click="refreshBalance"
          >
            <RefreshIcon />
          </Button>
          <PriceComp
            class="home-screen_balance text-[var(--text-color)] flex flex-col gap-[0]"
            :price="balance?.confirmed || 0"
            :loading-size="6"
            :loading="!isSynchronized"
            :font-size-breakpoints="{
              1000000: '40px',
              10000000: '30px',
              1000000000: '20px'
            }"
          />
        </template>
        <span
          class="home-screen_balance-label text-center text-[var(--text-grey-color)] mt-3"
        >
          {{ status_message }}
        </span>
        <template v-if="!connected">
        <Button
        outline
        @click="connectToSite"
        class="min-w-[40px] px-3 py-1 flex items-center justify-center bg-[var(--section)] text-[var(--text-color)]"
        >Connect to site</Button>
        </template>
      </div>

      <!-- Balance -->
      <div
        v-if="connected"
        class="home-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-4 gap-1"
      >
        <div class="flex items-center">
          <span class="mr-2 text-[var(--text-grey-color)]">Available:</span>
          <PriceComp
            class="ml-auto"
            :price="balance?.confirmed || 0"
            :loading="!isSynchronized"
            :font-size-breakpoints="{
              1000000: '15px'
            }"
          />
        </div>
        <div class="flex items-center hidden">
          <span class="mr-2 text-[var(--text-grey-color)]">Unconfirmed:</span>
          <PriceComp
            class="ml-auto"
            :price="(balance.unconfirmed < 0 ? 0 : balance.unconfirmed) || 0"
            :loading="!isSynchronized"
            :font-size-breakpoints="{
              1000000: '15px'
            }"
          />
        </div>
      </div>

      <!-- Info -->
      <div
        v-if="connected"
        class="home-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-4 gap-1"
      >
        <div class="flex items-center">
          <span class="mr-2 text-[var(--text-grey-color)]"
            >Used for inscription:</span
          >
          <PriceComp
            class="ml-auto"
            :price="balance?.used_for_inscribtions || 0"
            :loading="!isSynchronized"
            :font-size-breakpoints="{
              1000000: '15px'
            }"
          />
        </div>
        <div class="flex items-center">
          <span class="mr-2 text-[var(--text-grey-color)]"
            >Total inscriptions:</span
          >
          <p class="p-0 ml-auto text-[var(--text-color)]">
            {{ balance?.inscriptions?.length || 0 }} Units
          </p>
        </div>
      </div>

      <!-- Fund key -->
      <div
        class="home-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 gap-3 mt-auto"
      >
        <span class="mr-2 text-left text-[var(--text-grey-color)]"
          >To Add Funds Use This Address</span
        >
        <div class="flex items-center justify-between">
          <p class="p-0 mr-auto text-[var(--text-color)]">
            {{ formatAddress(fundAddress, 6, 6) }}
          </p>

          <label class="switch">
          <div class="label">{{ nameTypeAddress }}</div>
          <input type="checkbox"
            :checked="Boolean(typeAddress)"
            @click="toogleAddress">
            <span class="slider round"></span>
          </label>

          <Button
            outline
            @click="newFundAddress"
            class="min-w-[40px] mr-2 px-3 py-1 flex items-center justify-center bg-[var(--section)] text-[var(--text-color)]"
          >
            New
          </Button>
          <Button
            outline
            @click="copyToClipboard(fundAddress)"
            class="min-w-[40px] px-3 py-1 flex items-center justify-center bg-[var(--section)] text-[var(--text-color)]"
          >
            Copy
          </Button>
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full gap-3 mt-auto hidden">
        <Button disabled outline class="w-2/4">
          <img src="/assets/arrow-down.svg" class="mr-3" alt="Recieve" />
          <span>Recieve</span>
        </Button>
        <Button disabled class="w-2/4">
          <img src="/assets/arrow-up.svg" class="mr-3" alt="Send" />
          <span>Send</span>
        </Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { toRefs, computed } from 'vue'
import { sendMessage } from 'webext-bridge'
import { useStore } from '~/popup/store/index'
import RefreshIcon from '~/components/Icons/RefreshIcon.vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import {BALANCE_CHANGE_PRESUMED, NEW_FUND_ADDRESS, CHANGE_TYPE_FUND_ADDRESS, CONNECT_TO_SITE} from '~/config/events'
import useWallet from '~/popup/modules/useWallet'

const store = useStore()
const { balance, fundAddress, typeAddress } = toRefs(store)

const { getBalance, fetchUSDRate } = useWallet()

async function connectToSite() {
  await sendMessage(CONNECT_TO_SITE, {}, 'background')
  await refreshBalance()
}

async function toogleAddress(){

await sendMessage(BALANCE_CHANGE_PRESUMED, {}, 'background')
let ta = Number(!typeAddress.value);
store.setTypeAddress(ta);
const response = await sendMessage(
  CHANGE_TYPE_FUND_ADDRESS,
  {
    type: ta
  },
  'background'
  )
const addr = response?.addresses?.find(
  (item) => item.type === 'fund'
)?.address
store.setFundAddress(addr)

}

async function newFundAddress() {
  await sendMessage(BALANCE_CHANGE_PRESUMED, {}, 'background')
  const response = await sendMessage(NEW_FUND_ADDRESS, {}, 'background')
  const addr = response?.addresses?.find(
    (item) => item.type === 'fund'
  )?.address
  store.setFundAddress(addr)
}

function refreshBalance() {
  store.setSyncToFalse()
  fetchUSDRate()
  setTimeout(async () => {
    await getBalance(fundAddress.value);
  }, 1000)
}

const isSynchronized = computed(() => balance?.value?.sync)
const connected = computed(() => balance?.value?.connect)

const nameTypeAddress = computed(() => (typeAddress?.value===1)?'P2WPKH':'P2TR')

const status_message = computed(() => {
  if (!balance?.value?.connect)
    return 'On the site, press "Connect to wallet" button or'
  if (!balance?.value?.sync) return 'Synchronizing...'
  if (balance?.value?.confirmed > 0 || balance?.value?.sync)
    return 'Synchronized.'
})
</script>

<style lang="scss" scoped>
.home-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;
  }
  & p {
    font-weight: 600;
    font-size: 15px;
    line-height: 20px;
    display: flex;
    align-items: center;
    text-align: right;
    letter-spacing: -0.32px;
  }

  &_block span {
    font-weight: 400;
    font-size: 14px;
    line-height: 18px;
    letter-spacing: -0.154px;
  }

  &_balance {
    font-weight: 600;
    line-height: normal;

    &-refresh {
      width: 25px;
      height: 25px;
      padding: 5px;
    }

    &-label {
      font-weight: 400;
      font-size: 15px;
      line-height: 20px;
      display: flex;
      align-items: left;
      letter-spacing: -0.32px;
    }
  }
}

.label{
  margin-top: -1.1rem;
  font-size: 0.6rem;
}

.switch {
  margin-right: 0.5rem;
  position: relative;
  display: inline-block;
  width: 36px;
  height: 20px;
}

.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #ccc;
  -webkit-transition: .4s;
  transition: .4s;
}

.slider:before {
  position: absolute;
  content: "";
  height: 15px;
  width: 15px;
  left: 2px;
  bottom: 2px;
  background-color: white;
  -webkit-transition: .4s;
  transition: .4s;
}

input:checked + .slider {
  background-color: #6d7175;
}

input:focus + .slider {
  box-shadow: 0 0 1px #6d7175;
}

input:checked + .slider:before {
  -webkit-transform: translateX(17px);
  -ms-transform: translateX(17px);
  transform: translateX(17px);
}

.slider.round {
  border-radius: 34px;
}

.slider.round:before {
  border-radius: 50%;
}
</style>
