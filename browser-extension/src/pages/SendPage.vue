<template>
<div class="load-screen flex flex-col h-full" data-testid="send-page">
  <Header />
  <Logo />
  <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
  <div class="load-screen_content h-full overflow-y-auto overflow-x-hidden flex flex-col items-center px-5 pb-5">
  <div class="flex flex-col w-full items-start mb-4">
    <h1 class="load-screen_title text-[var(--text-color)] mb-2"></h1>
  </div>
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <div class="sign-screen_block-input flex flex-col">
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >Send to</span
        >
        <CustomInput
          class="w-full"
          placeholder="Recipient wallet address"
        >
        </CustomInput>
      </div>
    </div>

    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <div class="sign-screen_block-input flex flex-col">
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >Amount</span
        >
        <CustomInput
          class="w-full"
          :placeholder="balance?.confirmed"
        >
        <span>
          Enter max ({{balance?.confirmed}})
        </span>
        </CustomInput>
      </div>
    </div>
    <!-- TX Info -->

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

    </div>
   </div>
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
