<template>
<!-- Total -->
<div
  class="transactions-screen_block w-full flex items-center p-3 mb-5"
  v-show="raws?.length > 0"
  data-testid="get-raw-tx-description"
>
  <div @click="openTransactionsDetails = !openTransactionsDetails" class="flex items-center gap-2 mb-3 cursor-pointer w-full" data-testid="transactions-details">
    <ChevronIcon class="w-[20px]" :class="{ 'transform rotate-180': openTransactionsDetails }" />
    <span class="text-[15px] text-[var(--text-color)]">Transactions details</span>
  </div>

</div>

<div
  class="transactions-screen_block w-full flex items-center"
  style="display:grid;width: 90%;margin-left: 60px;"
  v-if="openTransactionsDetails"
  v-for="(item, index) in raws"
  data-testid="get-raw-tx-description"
>
  <div
    @click="openTransactionDetails[index] = !openTransactionDetails[index]"
    class="flex items-center gap-2 mb-3 cursor-pointer w-full"
    data-testid="transaction-details"
  >
    <ChevronIcon class="w-[20px]" :class="{ 'transform rotate-180': openTransactionDetails[index] }" />
    <div style="display:contents;">
    <span
    class="text-[15px] text-[var(--text-color)]"
    style="width: 100%;text-align: left;">Transcation {{ index+1 }}</span>
    <CopyIcon
          class="cursor-pointer"
          @click="copyToClipboard(raws[index], 'Constant was copied!')"
          v-if="openTransactionDetails[index]"
        />
    </div>
  </div>

  <div class="flex items-center mb-2">

    <CustomInput
    type="textarea"
    class="w-full"
    rows="3"
    v-model="raws[index]"
    readonly
    data-testid="raw-tx-dump"
    v-if="openTransactionDetails[index]"
    />
  </div>
</div>
</template>

<script setup lang="ts">
import { toRefs, computed } from 'vue'
import { copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import CopyIcon from '~/components/Icons/CopyIcon.vue'

const store = useStore()
const { dataForSign } = toRefs(store)


const openTransactionsDetails = ref(false)
const openTransactionDetails = ref([])

const raws = computed(() => {
  return dataForSign.value?.data?.costs?.raw || dataForSign.value?.data?.raw
})

</script>

<style scoped>
[active=true] {
  color: var(--text-grey-color);
}
.sign-screen_block span {
  text-align: left;
  font-weight: 400;
  font-size: 14px;
  line-height: 18px;
  letter-spacing: -0.154px;
}
</style>
