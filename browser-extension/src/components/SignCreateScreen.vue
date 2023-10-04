<template>
  <SignWrapper>
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
          :price="
            Math.abs(dataForSign?.data?.costs?.amount -
              dataForSign?.data?.costs?.expect_amount) || 0
          "
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
          :price="dataForSign?.data?.costs?.amount || 0"
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
    <!-- Total -->
    <div
      class="sign-screen_block w-full flex items-center bg-[var(--section)] rounded-lg p-3 mb-5"
    >
      <span><a @click="whatSigning" class="mr-2 text-[var(--text-color)]">What am I signing?</a></span>
      <span class="w-full text-[var(--text-grey-color)]"
        >Description:</span
      >
      <span class="mr-2 text-[var(--text-color)]" >...</span>
    </div>
    <!-- What am I signing -->
    <div
      class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
      v-if="showContract"
    >
      <div class="flex items-center mb-2">
        <span class="w-full text-[var(--text-grey-color)]"
          >Raw transcation:</span
        >
  <!--      <RefreshIcon @click="getJSON" class="cursor-pointer w-4 mr-2" /> -->
        <CopyIcon
          class="cursor-pointer"
          @click="copyToClipboard(textarea, 'Constant was copied!')"
        />
      </div>
      <span style="float:left;"><a @click="showRawTranscation(0)" class="mr-2 text-[var(--text-color)]">#raw_transcation_1</a></span>
      <span style="float:left;"><a @click="showRawTranscation(1)" class="mr-2 text-[var(--text-color)]">#raw_transcation_2</a></span>
      <CustomInput
        type="textarea"
        class="w-full"
        rows="3"
        v-model="textarea"
        readonly
      />
    </div>

    <!-- Inputs -->
  </SignWrapper>
</template>

<script setup lang="ts">
import { toRefs, computed } from 'vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import SignWrapper from '~/components/SignWrapper.vue'
import CopyIcon from '~/components/Icons/CopyIcon.vue'

const store = useStore()
const { balance, fundAddress, ordAddress, dataForSign } = toRefs(store)

const textarea = ref('')

const toAddress = computed(() => {
  return ordAddress.value || fundAddress.value
})

const showContract = ref('')
const isInsufficientBalance = computed(() => {
  if (Number(dataForSign.value.data?.costs?.amount) > Number(balance.value?.confirmed)) return true
  return false
})
async function showRawTranscation(n){
  showContract.value = true;
  textarea.value = dataForSign.value?.data?.costs?.raw[n]
}
async function whatSigning(){
  if(showContract.value){
    showContract.value = false;
    textarea.value = '';
    return;
  }
  showContract.value = true;
  textarea.value = dataForSign.value?.data?.costs?.raw[0]
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
