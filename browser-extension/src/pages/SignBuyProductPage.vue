<template>
  <SignWrapper data-testid="sign-buy-product-page">
    <!-- TX Info -->
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5 gap-3"
    >
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Owner Fee:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.owner_fee || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
          data-testid="platform-fee"
        />
      </div>
      <div class="flex items-center" v-if="dataForSign?.data?.market_fee > 0">
        <span class="mr-2 text-[var(--text-grey-color)]">Platform Fee:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.market_fee || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
          data-testid="platform-fee"
        />
      </div>
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Mining Fee:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.costs?.mining_fee || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
          data-testid="mining-fee"
        />
      </div>
      <div
        class="w-full min-h-[1px] bg-[var(--border-color)] dark:bg-[#555555]"
      />
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-color)]">Total Needed:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.costs?.min_fund_amount || total"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
          data-testid="total-needed"
        />
      </div>
    </div>

    <div
      class="sign-screen_block w-full flex items-center bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <span class="mr-2 text-[var(--text-color)]">Available:</span>
      <PriceComp
        class="ml-auto"
        :price="balance?.confirmed || 0"
        :loading="!isSynchronized"
        :font-size-breakpoints="{
          1000000: '15px'
        }"
        data-testid="available"
      />
    </div>
    <div
      class="sign-screen_block w-full flex items-center bg-[var(--section)] rounded-lg p-3 mb-5"
      v-if="balance?.unconfirmed > 0"
    >
      <span class="mr-2 text-[var(--text-color)]">Your funds are still awaiting confirmation</span><hr />
      </div><div
        class="sign-screen_block w-full flex items-center bg-[var(--section)] rounded-lg p-3 mb-5"
        v-if="balance?.unconfirmed > 0"
      >
      <span class="mr-2 text-[var(--text-color)]">Unconfirmed:</span>
      <PriceComp
        class="ml-auto"
        :price="balance?.unconfirmed || 0"
        :loading="!isSynchronized"
        :font-size-breakpoints="{
          1000000: '15px'
        }"
        data-testid="unconfirmed"
      />
    </div>
    <NotifyInBody/>
    <GetRawTransactions/>
  </SignWrapper>
</template>

<script setup lang="ts">
import { toRefs, computed } from 'vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import SignWrapper from '~/components/SignWrapper.vue'
import CopyIcon from '~/components/Icons/CopyIcon.vue'
import GetRawTransactions from '~/components/GetRawTransactions.vue'
import NotifyInBody from '~/components/NotifyInBody.vue'

const store = useStore()
const { balance, dataForSign, ordAddress } = toRefs(store)

const total = computed(
  () =>
    (dataForSign.value?.data?.owner_fee || 0) +
    (dataForSign.value?.data?.market_fee || 0)
)

const isSynchronized = computed(() => balance?.value?.sync)
const connected = computed(() => balance?.value?.connect)

const isInsufficientBalance = computed(() => {
  if (Number(total.value) > Number(balance.value?.confirmed)) return true
  return false
})

</script>

<style scoped>
.sign-screen_block span {
  text-align: left;
  font-weight: 400;
  font-size: 14px;
  line-height: 18px;
  letter-spacing: -0.154px;
}
</style>
