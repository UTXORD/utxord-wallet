<template>
  <SignWrapper>
    <div
      class="sign-screen_block w-full flex items-center bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <span class="mr-2 text-[var(--text-grey-color)]">Available Balance:</span>
      <PriceComp
        class="ml-auto"
        :price="balance?.confirmed || 0"
        :font-size-breakpoints="{
          1000000: '15px'
        }"
      />
    </div>
    <!-- To address -->
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <div class="sign-screen_block-input flex flex-col">
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >My funding address:</span
        >
        <CustomInput
          :value="formatAddress(fundAddress, 12, 12)"
          class="w-full"
          readonly
        >
          <CopyIcon
            class="cursor-pointer"
            @click="copyToClipboard(fundAddress)"
          />
        </CustomInput>
      </div>
    </div>

    <!-- TX Info -->
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5 gap-3"
    >
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Selling for:</span>
        <PriceComp
          class="ml-auto"
          :price="btcToSat(dataForSign?.data?.ord_price) || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
        />
      </div>
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Platform Fee:</span>
        <PriceComp
          class="ml-auto"
          :price="btcToSat(dataForSign?.data?.market_fee) || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
        />
      </div>
    </div>
    <!-- Total -->
  </SignWrapper>
</template>

<script setup lang="ts">
import { toRefs } from 'vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import SignWrapper from '~/components/SignWrapper.vue'
import CopyIcon from '~/components/Icons/CopyIcon.vue'

const store = useStore()
const { balance, fundAddress, dataForSign } = toRefs(store)

function btcToSat(btc) {
  return Math.floor(Number(btc) * Math.pow(10, 8))
}

function satToBtc(sat) {
  return Number(sat) / Math.pow(10, 8)
}
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
