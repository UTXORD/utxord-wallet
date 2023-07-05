<template>
  <div class="home-screen flex flex-col h-full">
    <Header />
    <Logo />
    <div
      class="home-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <!-- Balance -->
      <div
        class="home-screen_block w-full flex flex-col items-center bg-white rounded-lg px-3 py-5 mb-5"
      >
        <PriceComp
          class="home-screen_balance"
          :price="balance?.confirmed || 0"
          :font-size-breakpoints="{
            1000000: '40px',
            10000000: '30px',
            1000000000: '20px'
          }"
        />
        <span class="home-screen_balance-label text-center">
          {{ status }}
        </span>
      </div>

      <!-- Balance -->
      <div
        class="home-screen_block w-full flex flex-col bg-white rounded-lg p-3 mb-5 gap-3"
      >
        <div class="flex items-center">
          <span class="mr-2">Avaliable:</span>
          <PriceComp
            class="ml-auto"
            :price="balance?.confirmed || 0"
            :font-size-breakpoints="{
              1000000: '15px'
            }"
          />
        </div>
        <div class="flex items-center">
          <span class="mr-2">Unconfirmed:</span>
          <PriceComp
            class="ml-auto"
            :price="(balance.unconfirmed < 0 ? 0 : balance.unconfirmed) || 0"
            :font-size-breakpoints="{
              1000000: '15px'
            }"
          />
        </div>
      </div>

      <!-- Info -->
      <div
        class="home-screen_block w-full flex flex-col bg-white rounded-lg p-3 mb-5 gap-3"
      >
        <div class="flex items-center">
          <span class="mr-2">Used for inscription:</span>
          <PriceComp
            class="ml-auto"
            :price="balance?.used_for_inscribtions || 0"
            :font-size-breakpoints="{
              1000000: '15px'
            }"
          />
        </div>
        <div class="flex items-center">
          <span class="mr-2">Total inscriptions:</span>
          <p class="p-0 ml-auto">
            {{ balance?.inscriptions?.length || 0 }} Units
          </p>
        </div>
      </div>

      <!-- Fund key -->
      <div
        class="home-screen_block w-full flex flex-col bg-white rounded-lg p-3 gap-3 mt-auto"
      >
        <span class="mr-2 text-left">To Add Funds Use This Address</span>
        <div class="flex items-center justify-between">
          <p class="p-0 mr-auto">{{ formatAddress(fundAddress, 6, 6) }}</p>
          <Button
            outline
            @click="newFundAddress"
            class="min-w-[40px] mr-2 px-3 py-1 flex items-center justify-center bg-white text-black"
          >
            New
          </Button>
          <Button
            outline
            @click="copyToClipboard(fundAddress)"
            class="min-w-[40px] px-3 py-1 flex items-center justify-center bg-white text-black"
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
import { toRefs } from 'vue'
import { sendMessage } from 'webext-bridge'
import { useStore } from '~/popup/store/index'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { NEW_FUND_ADDRESS } from '~/config/events'

const store = useStore()
const { balance, fundAddress } = toRefs(store)

async function newFundAddress() {
  const response = await sendMessage(NEW_FUND_ADDRESS, {}, 'background')
  const addr = response?.addresses?.find(
    (item) => item.type === 'fund'
  )?.address
  store.setFundAddress(addr)
}
const status = computed(() => {
  if (balance?.value?.confirmed > 0) return 'Synchronized.'
  if (balance?.value?.sync) return 'Synchronizing...'
  return 'On the site, press "Connect to wallet" button.'
})
</script>

<style lang="scss" scoped>
.home-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;
    border-top: 1px solid #e8e8e8;
  }

  & p {
    font-weight: 600;
    font-size: 15px;
    line-height: 20px;
    display: flex;
    align-items: center;
    text-align: right;
    letter-spacing: -0.32px;
    color: #000000;
  }

  &_block span {
    font-weight: 400;
    font-size: 14px;
    line-height: 18px;
    letter-spacing: -0.154px;
    color: #6d7885;
  }

  &_balance {
    font-weight: 600;
    line-height: 55px;
    color: #000000;

    &-label {
      font-weight: 400;
      font-size: 15px;
      line-height: 20px;
      display: flex;
      align-items: left;
      letter-spacing: -0.32px;
      color: #737375;
    }
  }
}
</style>
