<template>
  <SignWrapper>
    <div
      class="sign-screen_block w-full flex items-center bg-white rounded-lg p-3 mb-5"
    >
      <span class="mr-2">Available Balance:</span>
      <PriceComp
        class="ml-auto"
        :price="balance?.confirmed || 0"
        :font-size-breakpoints="{
          1000000: '15px'
        }"
      />
    </div>
    <div
      class="sign-screen_block w-full flex flex-col bg-white rounded-lg p-3 mb-5"
    >
      <div class="sign-screen_block-input flex flex-col">
        <span class="mb-2 w-full">My ordinal address:</span>
        <CustomInput
          :value="formatAddress(ordAddress, 12, 12)"
          class="w-full"
          readonly
        >
          <img
            class="cursor-pointer"
            src="/assets/copy.svg"
            alt="Copy"
            @click="copyToClipboard(ordAddress)"
          />
        </CustomInput>
      </div>
    </div>

    <!-- TX Info -->
    <div
      class="sign-screen_block w-full flex flex-col bg-white rounded-lg p-3 mb-5 gap-3"
    >
      <div class="flex items-center">
        <span class="mr-2">Purchase Price:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.ord_price || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
        />
      </div>
      <div class="flex items-center">
        <span class="mr-2">Platform Fee:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.market_fee || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
        />
      </div>
      <div class="flex items-center">
        <span class="mr-2">Mining Fee:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.costs?.mining_fee || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
        />
      </div>
      <div class="sign-screen_block-separator" />
      <div class="flex items-center">
        <span class="mr-2">Total:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.costs?.min_fund_amount || 0"
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
import { toRefs, computed } from 'vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import SignWrapper from '~/components/SignWrapper.vue'

const store = useStore()
const { balance, dataForSign, ordAddress } = toRefs(store)
</script>

<style scoped>
.sign-screen p {
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  display: flex;
  align-items: center;
  text-align: right;
  letter-spacing: -0.32px;
  color: #000000;
}

.sign-screen_block span {
  text-align: left;
  font-weight: 400;
  font-size: 14px;
  line-height: 18px;
  letter-spacing: -0.154px;
  color: #6d7885;
}

.sign-screen_block-input input {
  background: #fafafa;
  border: 1px solid #ededed;
  border-radius: 4px;
  font-weight: 400;
  font-size: 15px;
  line-height: 20px;
  display: flex;
  align-items: center;
  letter-spacing: -0.32px;
  color: #000000;
  padding: 12px;
}

.sign-screen_block-separator {
  width: 100%;
  height: 1px;
  background: #e8e8e8;
}
</style>
