<template>
  <EstimateWrapper data-testid="estimate-fee-page">

    <!-- TX Info -->
    <NotifyInBody/>

  </EstimateWrapper>


</template>

<script setup lang="ts">
import { toRefs, computed } from 'vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
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
