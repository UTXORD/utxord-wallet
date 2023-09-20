<template>
  <SignWrapper>
<!--
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <div class="sign-screen_block-input flex flex-col">
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >My ordinal address:</span
        >
        <CustomInput
          :value="formatAddress(ordAddress, 12, 12)"
          class="w-full"
          readonly
        >
          <CopyIcon
            class="cursor-pointer"
            @click="copyToClipboard(ordAddress)"
          />
        </CustomInput>
      </div>
    </div>
-->
    <!-- TX Info -->
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5 gap-3"
    >
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Purchase Price:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.ord_price || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
        />
      </div>
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Platform Fee:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.market_fee || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
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
        :font-size-breakpoints="{
          1000000: '15px'
        }"
      />
    </div>
    <div v-show="isInsufficientBalance"
      class="sign-screen_block w-full flex items-center bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <span class="mr-2 text-[var(--text-color)]" style="font-size: 20px;">Insufficient funds. Please add.</span>

    </div>
  </SignWrapper>
</template>

<script setup lang="ts">
import { toRefs } from 'vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import SignWrapper from '~/components/SignWrapper.vue'
import CopyIcon from '~/components/Icons/CopyIcon.vue'

const store = useStore()
const { balance, dataForSign, ordAddress } = toRefs(store)

const total = computed(
  () =>
    (dataForSign.value?.data?.ord_price || 0) +
    (dataForSign.value?.data?.market_fee || 0)
)

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
