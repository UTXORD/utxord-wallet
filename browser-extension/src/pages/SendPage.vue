<template>
<div class="load-screen flex flex-col h-full" data-testid="send-page">
  <Header />
  <Logo />
  <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
  <div class="load-screen_content h-full overflow-y-auto overflow-x-hidden flex flex-col items-center px-5 pb-5">
  <div class="flex flex-col w-full items-start mb-4">
    <h1 class="load-screen_title text-[var(--text-color)] mb-2"></h1>
  </div>
  <div class="sign-screen_block-title-wrapper w-full mb-2 flex flex-col">
    <span class="sign-screen_block-title-text text-[var(--text-color)] w-full"
      >Send to</span
    >
    </div>
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <span v-if="AddressInfo?.length > 0" class="mr-2 p-2 text-[var(--text-grey-color)]">{{AddressInfo}}</span>
      <div class="sign-screen_block-input flex flex-col">

        <CustomInput
          class="w-full"
          placeholder="Recipient wallet address"
          v-model="address"
        >
        </CustomInput>
      </div>
    </div>
    <div class="sign-screen_block-title-wrapper flex flex-col w-full">
    <span class="sign-screen_block-title-text text-[var(--text-color)] amount-title"
      >Amount</span
    >
    <span
    class="sign-screen_block-title-emax"
    @click="enterMax"
    >
      Enter max ({{toNumberFormat(balance?.confirmed)}})
    </span>
    </div>
    <div
      class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <div class="sign-screen_block-input flex flex-col">
          <CustomInput
            class="w-full"
            :placeholder="placeholderAmount"
            after="sat"
            v-model="amount"
            type="number"
            min="546"
            :max="balance?.confirmed"
          >
          </CustomInput>
      </div>
    </div>
    <!-- TX Info -->

    <div
      class="sign-screen_block w-full flex items-center rounded-lg p-3 mb-6"
    >
      <span class="mr-2 text-[var(--text-grey-color)]">Available balance</span>
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

      <!-- Buttons -->
      <div class="flex w-full mb-5 mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center"
          @click="goToBack"
          data-testid="go-back"
        >
          <ArrowLeftIcon />
        </Button>
        <Button @click="onContinue" :disabled="isDisabled" enter class="w-full" data-testid="continue">
          Continue
        </Button>
      </div>
    </div>
   </div>
</template>

<script setup lang="ts">
import {toRefs, ref, computed, onMounted} from 'vue'
import {
  formatAddress,
  copyToClipboard,
  toNumberFormat,
  validateBtcAddress,
  validateBtcAddressInfo
  } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import SignWrapper from '~/components/SignWrapper.vue'
import CopyIcon from '~/components/Icons/CopyIcon.vue'
import GetRawTransactions from '~/components/GetRawTransactions.vue'
import NotifyInBody from '~/components/NotifyInBody.vue'
import { useRouter } from 'vue-router'

const { back, push } = useRouter()

const store = useStore()
const { balance, fundAddress, ordAddress, dataForSign } = toRefs(store)

const isSynchronized = computed(() => balance?.value?.sync)
const connected = computed(() => balance?.value?.connect)

// const miningFee = computed(
//   () => Math.abs(dataForSign.value?.data?.costs?.amount -
//     dataForSign.value?.data?.costs?.expect_amount) || 0
// )
const address = ref('')
const amount = ref('')
//const totalNeed = computed(() => dataForSign.value?.data?.costs?.amount ||  0)

function goToBack(){
  push('/')
}

function enterMax(){
  amount.value = Number(balance.value?.confirmed)
}

const isDisabled = computed(() => {
  if (!address.value?.length) return true
  if (amount.value < 546 || amount.value > Number(balance.value?.confirmed)) return true
  const valid = validateBtcAddress(address.value)
  if(!valid) return true;
  return false
})

const AddressInfo = computed(() =>{
  if(address.value.length > 20){
    return validateBtcAddressInfo(address.value)
  }
  return ''
})

const placeholderAmount = computed(() => `From 546 to ${toNumberFormat(balance.value?.confirmed)}`)

function onContinue(){
 dataForSign.value.location = '/confirm-send-to'
 dataForSign.value.amount = amount
 dataForSign.value.address = address
 console.log('dataForSign:', dataForSign.value);
 push('/estimate-fee')
}

onMounted(() => {
  if(!dataForSign.value){
    dataForSign.value = {};
  }
  if(dataForSign.value?.location) dataForSign.value.location = undefined;
  console.log('dataForSign.value', dataForSign.value);
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
.sign-screen_block-title-text {
  text-align: left;
  font-size: medium;
}
.sign-screen_block-title-emax{
  text-align: right;
  font-size: small;
  color: #00e494;
  cursor: pointer;
  z-index: 2;
}
.amount-title {
  position: relative;
  top: 1rem;
  z-index: 1;
}
</style>
