<template>
  <div class="home-screen flex flex-col h-full" data-testid="home-page">
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
            class="home-screen_balance-refresh absolute top-2 right-2 !min-h-[20px]"
            @click="refreshBalance"
            data-testid="refresh-balance"
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
            data-testid="balance-confirmed"
          />
        </template>
        <span
          class="home-screen_balance-label text-center text-[var(--text-grey-color)]"
        >
          {{ status_message }}
        </span>
        <template v-if="!connected">
        <Button
          outline
          @click="connectToSite"
          class="min-w-[40px] px-3 py-1 mt-1 flex items-center justify-center bg-[var(--section)] text-[var(--text-color)]"
          data-testid="connect-to-site"
        >Connect to site</Button>
        </template>

      <div>
  <!--      <span
           v-if="Number(balance?.confirmed) > 0"
          class=" w-2/4"
          data-testid="send"
          @click="sendTo"
          style="cursor: pointer;font-size: 20px;margin-top: 5px;"
          >
          &#9658;
        </span>-->
        <span
          class=" w-2/4"
          data-testid="addresses"
          @click="addressesList"
          style="cursor: pointer;font-size: 20px;margin-top: 5px;"
          >
          &#8801;
        </span>
        </div>
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
            data-testid="balance-available"
          />
        </div>
        <div class="flex items-center" v-if="balance?.unconfirmed > 0">
          <span class="mr-2 text-[var(--text-grey-color)]">Unconfirmed:</span>
          <PriceComp
            class="ml-auto"
            :price="balance?.unconfirmed || 0"
            :loading="!isSynchronized"
            :font-size-breakpoints="{
              1000000: '15px'
            }"
            data-testid="balance-unconfirmed"
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
            data-testid="used-for-inscription"
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
        <div class="flex items-center gap-2">
          <span class="mr-auto text-left text-[var(--text-grey-color)]">
            To add funds use this address
          </span>
          <RefreshIcon
            v-if="useDerivation"
            class="w-4 h-4 clickable"
            @click="newFundAddress"
          />
          <CopyIcon
            class="clickable"
            @click="copyToClipboard(fundAddress, 'Address was copied!')"
          />
        </div>
        <div class="flex items-center justify-between">
          <p class="p-0 mr-auto text-[var(--text-color)]">
            {{ formatAddress(fundAddress, 15, 15) }}
          </p>
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full gap-3 mt-auto hidden">
        <Button disabled outline class="w-2/4" data-testid="receive">
          <img src="/assets/arrow-down.svg" class="mr-3" alt="Receive" />
          <span>Receive</span>
        </Button>
        <Button disabled class="w-2/4" data-testid="send">
          <img src="/assets/arrow-up.svg" class="mr-3" alt="Send" />
          <span>Send</span>
        </Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { toRefs, computed } from 'vue'
import { useStore } from '~/popup/store/index'
import RefreshIcon from '~/components/Icons/RefreshIcon.vue'
import CopyIcon from '~/components/Icons/CopyIcon.vue'
import { formatAddress, copyToClipboard, sendMessage } from '~/helpers/index'
import {BALANCE_CHANGE_PRESUMED, NEW_FUND_ADDRESS, CHANGE_TYPE_FUND_ADDRESS, STATUS_DERIVATION, CONNECT_TO_SITE} from '~/config/events'
import useWallet from '~/popup/modules/useWallet'
import { useRouter } from 'vue-router'
const { back, push } = useRouter()

const store = useStore()
const { balance, fundAddress, typeAddress, useDerivation } = toRefs(store)

const { getBalance, getFundAddress, fetchUSDRate } = useWallet()

async function connectToSite() {
  await sendMessage(CONNECT_TO_SITE, {}, 'background')
  await refreshBalance()
}

async function newFundAddress() {
  await sendMessage(BALANCE_CHANGE_PRESUMED, {}, 'background')
  const response = await sendMessage(NEW_FUND_ADDRESS, {}, 'background')
  const ta = Number(typeAddress.value);
  const tl = Boolean(useDerivation.value)?'fund':'oth'
  const addr = response?.addresses?.reverse()?.find(
    (item) => item.type === tl && item.typeAddress === ta
  )?.address
  store.setFundAddress(addr)
}

function sendTo(){
  return push('/send-to')
}

function addressesList(){
  return push('/addresses')
}

function refreshBalance() {
  store.setSyncToFalse()
  fetchUSDRate()
  setTimeout(async () => {
    const res = await sendMessage(STATUS_DERIVATION, {}, 'background')
    store.setUseDerivation(Boolean(res.derivate))
    const ta = Number(typeAddress.value);
    const tl = Boolean(useDerivation.value)?'fund':'oth'
    const addr = res.keys?.addresses?.reverse()?.find(
      (item) => item.type === tl && item.typeAddress === ta
    )?.address
    await store.setFundAddress(addr)
    await getBalance(fundAddress.value);
  }, 500)
}

const isSynchronized = computed(() => balance?.value?.sync)

const connected = computed(() => balance?.value?.connect)

const status_message = computed(() => {
  if (!balance?.value?.connect)
    return 'On the site, press "Connect to wallet" button or'
  if (!balance?.value?.sync) return 'Synchronizing...'
  if (balance?.value?.confirmed > 0 || balance?.value?.sync)
    return 'Synchronized.'
})
onBeforeMount(async() => {
  console.log('onBeforeMount')
  await getFundAddress()
  await refreshBalance();
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
