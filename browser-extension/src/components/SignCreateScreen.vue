<template>
  <SignWrapper>
    <!-- TX Info -->
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5 gap-3"
    >
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Inscribing on:</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.data?.costs?.expect_amount || 0"
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
          :price="miningFee"
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
          :price="totalNeed"
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
        :loading="!isSynchronized"
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
    <GetRawTransactions/>
    <!-- Inputs -->
  </SignWrapper>
</template>

<script setup lang="ts">
import { toRefs, computed } from 'vue'
import { useStore } from '~/popup/store/index'
import SignWrapper from '~/components/SignWrapper.vue'
import GetRawTransactions from '~/components/GetRawTransactions.vue'


const store = useStore()
const { balance, dataForSign } = toRefs(store)

const isSynchronized = computed(() => balance?.value?.sync)
const connected = computed(() => balance?.value?.connect)

const miningFee = computed(
  () => Math.abs(dataForSign.value?.data?.costs?.amount -
    dataForSign.value?.data?.costs?.expect_amount) || 0
)

const totalNeed = computed(() => dataForSign.value?.data?.costs?.amount ||  0)

const isInsufficientBalance = computed(() => {
  if (Number(totalNeed.value) > Number(balance.value?.confirmed)) return true
  return false
})

const toAddress = computed(() => {
  return ordAddress.value || fundAddress.value
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
