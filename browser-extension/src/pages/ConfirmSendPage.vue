<template>
  <SignWrapperForSend data-testid="confirm-send-page">
    <!-- To address -->
    <!-- Title and description -->
    <div class="w-full flex flex-col gap-2 mb-4">
      <div class="sign-screen_block-title">
        Rewiew
      </div>
    </div>

    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
    >
    <PriceComp
      class="sign-screen_amount text-[var(--text-color)] flex flex-col gap-[0]"
      :price="dataForSign?.amount || 0"
      :loading-size="6"
      :font-size-breakpoints="{
        1000000: '40px',
        10000000: '30px',
        1000000000: '20px'
      }"
      data-testid="amount"
    />

    </div>
    <div
      class="sign-screen_block-wrapper w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
    >

    <span class="sign-screen_block-title text-[var(--text-grey-color)] address-title"
      >To&nbsp;&nbsp;&nbsp;</span
    >
    <span class="sign-screen_block-address text-[var(--text-color)] address"
      >{{dataForSign?.address}}</span
    >
    </div>

    <div
      class="sign-screen_block w-full flex flex-col rounded-lg p-3 mb-5 gap-3"
    >
      <div class="flex items-center">
        <TextComp
          class="ml-auto"
          :text="dataForSign?.data?.collection?.metadata?.title"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
          data-testid="block-title"
        />
      </div>
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Sending</span>
        <PriceComp
          class="ml-auto"
          :price="dataForSign?.amount || 0"
          :font-size-breakpoints="{
            1000000: '15px'
          }"
          data-testid="lay-setup-fee"
        />
      </div>
      <div class="flex items-center">
        <span class="mr-2 text-[var(--text-grey-color)]">Transaction Fee:</span>
        <PriceComp
          class="ml-auto"
          :price="miningFee || 0"
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
        <span class="mr-2 text-[var(--text-color)]">TOTAL</span>
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


    <NotifyInBody/>
    <GetRawTransactions/>
    <!-- Inputs -->
  </SignWrapperForSend>
</template>

<script setup lang="ts">
import {toRefs, ref, computed, onMounted, onBeforeMount} from 'vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import SignWrapperForSend from '~/components/SignWrapperForSend.vue'
import CopyIcon from '~/components/Icons/CopyIcon.vue'
import GetRawTransactions from '~/components/GetRawTransactions.vue'
import NotifyInBody from '~/components/NotifyInBody.vue'
import { SEND_TO } from '~/config/events'
import { sendMessage } from 'webext-bridge'
import useWallet from '~/popup/modules/useWallet'
const { getBalance, saveDataForSign } = useWallet()


const store = useStore()
const { balance, fundAddress, ordAddress, dataForSign } = toRefs(store)

const isSynchronized = computed(() => balance?.value?.sync)
const connected = computed(() => balance?.value?.connect)

onMounted(async() => {
  console.log('1');
})

onBeforeMount(async()=>{
console.log('2');
  if(!dataForSign.value){
    dataForSign.value = {};
  }
  console.log('dataForSign.value', dataForSign.value);
  const payload = await sendMessage(SEND_TO, dataForSign.value, 'background')
  console.log('payload.data:',payload.data);
  //  await saveDataForSign(payload.data);
  dataForSign.value = {
  ...dataForSign.value,
  ...payload
  };
  miningFee.value = await dataForSign.value?.data?.costs?.total_mining_fee
  totalNeed.value = (Number(dataForSign.value?.amount) + Number(miningFee.value));
  console.log('totalNeed.value:', totalNeed.value)
  console.log('miningFee.value:', miningFee.value)
  console.log(JSON.parse(payload.data.costs.data))

  if(dataForSign.value?.location) dataForSign.value.location = undefined;
  if(dataForSign.value?.back) dataForSign.value.back = undefined;
})

const miningFee = ref(0)
const totalNeed = ref(0)



</script>

<style lang="scss" scoped>
.sign-screen {
  &_block-title {
    flex: 1;
    font-size: 18px;
    line-height: 24.59px;
    text-align: left;
  }
  &_block-wrapper {
    display: inline-block;
  }
  &_block span {
    font-weight: 400;
    font-size: 14px;
    line-height: 18px;
    letter-spacing: -0.154px;
  }
  &_block-address {
    word-break: break-all;
    text-align: right;
  }
  &_amount {
    font-weight: 600;
    line-height: normal;

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
