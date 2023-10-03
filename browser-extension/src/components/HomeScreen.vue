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
            class="home-screen_balance text-[var(--text-color)]"
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
          class="home-screen_balance-label text-center text-[var(--text-grey-color)]"
        >
          {{ status_message }}
        </span>
      </div>

      <!-- Balance -->
      <div
        v-if="connected"
        class="home-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-4 gap-1"
      >
        <div class="flex items-center">
          <span class="mr-2 text-[var(--text-grey-color)]">Avaliable:</span>
          <PriceComp
            class="ml-auto"
            :price="balance?.confirmed || 0"
            :loading="!isSynchronized"
            :font-size-breakpoints="{
              1000000: '15px'
            }"
          />
        </div>
        <div class="flex items-center">
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
import {BALANCE_CHANGE_PRESUMED, NEW_FUND_ADDRESS} from '~/config/events'
import useWallet from '~/popup/modules/useWallet'

const store = useStore()
const { balance, fundAddress } = toRefs(store)

const { getBalance } = useWallet()

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
  setTimeout(() => {
    getBalance(fundAddress)
  }, 1000)
}

const isSynchronized = computed(() => balance?.value?.sync)
const connected = computed(() => balance?.value?.connect)

const status_message = computed(() => {
  if (!balance?.value?.connect)
    return 'On the site, press "Connect to wallet" button.'
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
    line-height: 55px;

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
</style>
