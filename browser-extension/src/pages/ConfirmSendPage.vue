<template>
  <SignWrapper data-testid="sign-transfer-page">
    <!-- To address -->
<!--
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <div class="sign-screen_block-input flex flex-col">
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >My ordinal address:</span
        >
        <CustomInput
          :value="formatAddress(toAddress, 12, 12)"
          class="w-full"
          readonly
        >
          <CopyIcon
            class="cursor-pointer"
            @click="copyToClipboard(toAddress)"
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
        <span class="mr-2 text-[var(--text-grey-color)]">Transfer collection:</span>
        <TextComp
          class="ml-auto"
          :text="dataForSign?.data?.collection?.metadata?.title"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
          data-testid="collection-title"
        />
      </div>
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Lazy setup fee:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.market_fee?.amount || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
          data-testid="lay-setup-fee"
        />
      </div>
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Transaction Mining Fee:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.costs?.total_mining_fee || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
          data-testid="tx-mining-fee"
        />
      </div>
      <div
        class="w-full min-h-[1px] bg-[var(--border-color)] dark:bg-[#555555]"
      />
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-color)]">Total Needed:</span>
        <PriceComp
          class="ml-auto"
          :price="totalNeed"
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

    <NotifyInBody/>
    <GetRawTransactions/>
    <!-- Inputs -->
  </SignWrapper>
</template>

<script setup lang="ts">
import {toRefs, computed, onMounted} from 'vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import SignWrapper from '~/components/SignWrapper.vue'
import CopyIcon from '~/components/Icons/CopyIcon.vue'
import GetRawTransactions from '~/components/GetRawTransactions.vue'
import NotifyInBody from '~/components/NotifyInBody.vue'


const store = useStore()
const { balance, fundAddress, ordAddress, dataForSign } = toRefs(store)

const isSynchronized = computed(() => balance?.value?.sync)
const connected = computed(() => balance?.value?.connect)

// const miningFee = computed(
//   () => Math.abs(dataForSign.value?.data?.costs?.amount -
//     dataForSign.value?.data?.costs?.expect_amount) || 0
// )

const totalNeed = computed(() => dataForSign.value?.data?.costs?.amount ||  0)

onMounted(() => {
  console.debug('dataForSign.value', dataForSign.value);
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
