<template>
  <SignWrapper data-testid="sign-transfer-page">
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5 gap-3"
    >
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
    <div
      class="sign-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
      data-testid="get-raw-tx-contract"
    >
      <div class="flex items-center mb-2">
        <span class="w-full text-[var(--text-grey-color)]"
          >Input your transaction:</span
        >
        <CopyIcon
          class="cursor-pointer"
          @click="copyToClipboard(transaction, 'Constant was copied!')"
        />
      </div>
      <span>
      <a v-for="(item, index) in raws" @click="showRawTranscation(index)" :active="activeTab === index" style="float:left;" class="mr-2 text-[var(--text-color)]" data-testid="raw-tx-link">#raw_transcation_{{ index+1 }}</a>
      </span>
      <CustomInput
        type="textarea"
        class="w-full"
        rows="3"
        v-model="transaction"
        data-testid="raw-tx-dump"
      />
    </div>
    <NotifyInBody/>
    <GetRawTransactions/>
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
import { sendMessage } from 'webext-bridge'

import {
  SIGN_SIMPLE_TRANSACTION,
  CREATE_SIMPLE_TRANSACTION
} from '~/config/events'

const store = useStore()
const { balance, fundAddress, ordAddress, dataForSign } = toRefs(store)

const transaction = ref('')
const isSynchronized = computed(() => balance?.value?.sync)
const connected = computed(() => balance?.value?.connect)


//const totalNeed = computed(() => dataForSign.value?.data?.costs?.amount ||  0)

onMounted(async() => {
const data = await sendMessage(
  CREATE_SIMPLE_TRANSACTION, {
    fee_rate: 1000,
  },
  'background'
  )
  dataForSign.value.data = data
  console.debug('dataForSign.value', dataForSign.value);
  transaction.value = dataForSign.value?.data?.raw
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
