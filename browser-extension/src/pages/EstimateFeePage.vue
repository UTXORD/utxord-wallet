<template>
  <EstimateWrapper data-testid="estimate-fee-page">

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
      <Button @click="onConfirm" enter class="w-full" data-testid="confirm">
        Confirm
      </Button>
    </div>

    <!-- TX Info -->
    <NotifyInBody/>
  </EstimateWrapper>


</template>

<script setup lang="ts">
import { toRefs, computed } from 'vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import SignWrapper from '~/components/SignWrapper.vue'
import CopyIcon from '~/components/Icons/CopyIcon.vue'
import GetRawTransactions from '~/components/GetRawTransactions.vue'
import NotifyInBody from '~/components/NotifyInBody.vue'
import {useRouter} from "vue-router";


const { back, push } = useRouter()


const store = useStore()
const { balance, dataForSign, ordAddress } = toRefs(store)

const total = computed(
  () =>
    (dataForSign.value?.data?.ord_price || 0) +
    (dataForSign.value?.data?.market_fee || 0)
)

function goToBack() {
  // removeTempDataFromLocalStorage()
  push('/start')
}

async function onConfirm() {
    // dataForSign.value = {...dataForSign.value, ...{selectedMiningFee: 0}};
    await push(`//sign-commit-buy`)
}


</script>

<style scoped>
.estimate-fee-screen_block span {
  text-align: left;
  font-weight: 400;
  font-size: 14px;
  line-height: 18px;
  letter-spacing: -0.154px;
}
</style>
