<template>
  <LoadingPage v-if="loading" />
  <div v-else class="manage-utxos flex flex-col h-full" data-testid="manage-page">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="manage-screen_content h-full flex flex-col items-start px-5">

      <h1 class="text-[var(--text-color)] text-[18px] mb-4 items-start ">Address details</h1>
      <h2 class="text-[var(--text-color)] text-[15px] mb-4 items-start " style="display: inline-flex;">
      {{formatAddress(page, 12, 12)}}
       <CopyIcon
          class="clickable"
          @click="copyToClipboard(page, 'Address was copied!')"
        />
      </h2>
      <div style="text-align: start;">
        <span class="mr-2 justify-between text-[15px] text-[var(--text-grey-color)]">Path:</span>
        <span class="mr-2 justify-between text-[15px]">{{ address?.index }}</span>
        <br />
        <span class="mr-2 justify-between text-[15px] text-[var(--text-grey-color)]">Available balance:</span>
        <span class="mr-2 justify-between text-[15px]">{{ availableBalance }}</span>
        <br />
        <span class="mr-2 justify-between text-[15px] text-[var(--text-grey-color)]">Total inscriptions:</span>
        <span class="mr-2 justify-between text-[15px]">{{ inscriptionsCount }}</span>
        <br />
        <span class="mr-2 justify-between text-[15px] text-[var(--text-grey-color)]">Used for inscription:</span>
        <span class="mr-2 justify-between text-[15px]">{{ usedForInscriptions }}</span>
        <br />
        <span class="mr-2 justify-between text-[15px] text-[var(--text-grey-color)]">Total runes:</span>
        <span class="mr-2 justify-between text-[15px]">{{ runesCount }}</span>
        <br />
        <span class="mr-2 justify-between text-[15px] text-[var(--text-grey-color)]">Used for runes:</span>
        <span class="mr-2 justify-between text-[15px]">{{ usedForRunes }}</span>
      </div>
      <br />
      <NotifyInBody/>
      <div class="w-full flex flex-col bg-[var(--section)] rounded-xl mb-5" style="text-align:left;">
      <div
      v-for="(item, i) in UTXOS"
      :key="i"
      class="border-b-[1px] p-4 border-[var(--border-color)] dark:border-[#4e4e4e]"
      >

      <span class="mr-2 justify-between text-[15px] text-[var(--text-grey-color)]">Amount:</span>
      <span class="mr-2 justify-between text-[15px]">{{ item?.amount }}</span><br />

     <div style="display: inline-flex;">
     <span class="mr-2 text-[var(--text-grey-color)]">
       txid:
     </span>
            <span
              class="mr-2 text-[var(--text-color)]"
            >
              {{ formatAddress(item?.txid, 15, 15) }}
            </span>
            <CopyIcon
              class="clickable"
              @click="copyToClipboard(item.txid, 'TxId was copied!')"
            />
     </div>
        <br />
        <span class="mr-2 text-[var(--text-grey-color)]">nout:</span>
        <span class="mr-2 text-left">{{ item?.nout }}</span>

        <span v-if="item?.in_queue" class="mr-2 text-[var(--text-grey-color)]">In Queue:</span>
        <span v-if="item?.in_queue" class="mr-2 text-left">{{ item?.in_queue }}</span>

        <span v-if="item?.is_confirmed" class="mr-2 text-[var(--text-grey-color)]">Is Confirmed:</span>
        <span v-if="item?.is_confirmed" class="mr-2 text-left">{{ item?.is_confirmed }}</span>

          <span v-if="item?.is_inscription" class="mr-2 text-[var(--text-grey-color)]">Is Inscription:</span>
          <span v-if="item?.is_inscription" class="mr-2 text-left">{{ item?.is_inscription }}</span><br />

          <span v-if="item?.is_locked" class="mr-2 text-[var(--text-grey-color)]">Is Locked:</span>
          <span v-if="item?.is_locked" class="mr-2 text-left">{{ item?.is_locked }}</span>

          <span v-if="item?.is_rune" class="mr-2 text-[var(--text-grey-color)]">Is Rune:</span>
          <span v-if="item?.is_rune" class="mr-2 text-left">{{ item?.is_rune }}</span>
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mt-auto">
        <Button
          second
          class="w-full px-0 flex items-center justify-center gap-2"
          @click="push('/addresses')"
          data-testid="go-back"
        >
          Go Back
        </Button>
      </div>
    </div>
  </div>

</template>

<script setup lang="ts">
import { toRefs, computed, ref } from 'vue'
import { sendMessage } from 'webext-bridge'
import { GET_ADDRESSES, GET_ALL_BALANCES } from '~/config/events'
import { useRouter } from 'vue-router'
import LoadingPage from '~/pages/LoadingPage.vue'
import { formatAddress, copyToClipboard } from '~/helpers/index'
import NotifyInBody from '~/components/NotifyInBody.vue'
const { back, push } = useRouter()

import { useStore } from '~/popup/store/index'
const store = useStore()
const { balance, dataForSign } = toRefs(store)

const availableBalance = ref(0)
const inscriptionsCount = ref(0)
const usedForInscriptions = ref(0)
const runesCount = ref(0)
const usedForRunes = ref(0)

const page = computed(() =>{
  const current_page = window?.history?.state?.current?.split('#')[1]
  if(!current_page) return push('/addresses')
  return current_page;
})



const loading = ref(true)
let connnectTry = 0
const UTXOS = ref([])
const address = ref({})


async function updateBalances(){
  const list = await sendMessage(GET_ADDRESSES, {refresh: true}, 'background')
  const utxoList = await sendMessage(GET_ALL_BALANCES, {}, 'background')
  console.log('list:', list)
  console.log('utxoList:', utxoList)
if(utxoList?.length > 0 && list?.addresses?.length > 0){
  for(let item of list?.addresses){
    if(item.address === page.value){
      address.value = item
    }
  }
console.log('address.value:',address.value)

    for(let u of utxoList){
      if(page.value === u.address){
      address.value['utxo_set'] = u.utxo_set;
      }
    }
  UTXOS.value = address.value?.utxo_set

  availableBalance.value = address.value?.utxo_set.reduce((acc, cur)=>{
    if(!cur?.is_locked && !cur?.is_inscription && !cur?.is_rune && !cur?.in_queue){
      return acc + Number(cur?.amount)
    }
    return acc
  },0)
  inscriptionsCount.value = address.value?.utxo_set.reduce((acc, cur)=>{
    if(cur?.is_inscription === true){
      return acc += 1
    }
    return acc
  },0)
  runesCount.value = address.value?.utxo_set.reduce((acc, cur)=>{
    if(cur?.is_rune === true){
      return acc += 1
    }
    return acc
  },0)
  usedForInscriptions.value = address.value?.utxo_set.reduce((acc, cur)=>{
    if(cur?.is_inscription === true){
      return acc + Number(cur?.amount)
    }
    return acc
  },0)
  usedForRunes.value = address.value?.utxo_set.reduce((acc, cur)=>{
    if(cur?.is_rune === true){
      return acc + Number(cur?.amount)
    }
    return acc
  },0)
  loading.value = false
  console.log('address.value:',address.value, 'UTXOS.value:',UTXOS.value)
  }else{
    setTimeout(async() => {
      connnectTry += 1;
    if(connnectTry < 3){
      await updateBalances()
      }else{
        if(balance.value?.confirmed > 0){
          dataForSign.value['data'] = {
              errorMessage: 'Unable to get balance, check your internet connection.',
          }
          loading.value = false
        }
      }
    }, 1000)
  }
}


onBeforeMount(async() => {

await updateBalances();

console.log(page.value)

})

</script>

<style lang="scss" scoped>
.manage-utxos {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;

    p {
      font-size: 18px;
      line-height: 25px;
      margin-bottom: 15px;
    }
  }

  &_form span {
    text-align: left;
    font-weight: 400;
    font-size: 14px;
    line-height: 18px;
    letter-spacing: -0.154px;
  }

  &_info {
    font-weight: 500;
    font-size: 15px;
    line-height: 20px;
    display: flex;
    letter-spacing: -0.32px;
    color: #1b46f5;
  }
}
</style>
