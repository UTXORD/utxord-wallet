<template>
  <LoadingPage v-if="loading" />
  <div v-else class="manage-addresses flex flex-col h-full" data-testid="manage-addresses-page">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="manage-addresses_content h-full flex flex-col items-start px-5">

      <h1 class="text-[var(--text-color)] text-[18px] mb-4">Addresses</h1>
  <div class="filters">
    <span class="mr-2 text-[var(--text-grey-color)]">
    Zero balance: <input type="checkbox" v-model="zero" name="zero" @click="updateBalances">
    Technical addresses: <input type="checkbox" v-model="teh" name="teh" @click="updateBalances">
    </span>
  </div>
      <NotifyInBody/>
      <div class="w-full flex flex-col bg-[var(--section)] rounded-xl mb-5" style="text-align:left;">
      <div
      v-for="(item, i) in LINKS"
      :key="i"
      class="border-b-[1px] p-4 border-[var(--border-color)] dark:border-[#4e4e4e]"
      >
     <div style="display: inline-flex;">
            <span
              v-if="item?.utxo_set"
              @click="goTo(item.address)"
              class="cursor-pointer justify-between text-[15px] text-[var(--text-color)] hover:text-[var(--text-blue)]"
            >
              {{ formatAddress(item.address, 12, 12) }}
            </span>
            <span
              v-else
              class="justify-between text-[15px] text-[var(--text-color)]"
            >
              {{ formatAddress(item.address, 12, 12) }}
            </span>
            <CopyIcon
              class="clickable"
              @click="copyToClipboard(item.address, 'Address was copied!')"
            />
     </div>
        <br />
        <span class="mr-2 text-[var(--text-grey-color)]">
          Path:
        </span>
          <span class="mr-2 text-left">{{ item.index }}</span>
          <span class="mr-2 text-[var(--text-grey-color)]">Space:</span>
          <span class="mr-2 text-left">{{ item.type }}</span><br />
          <span v-if="item?.utxo_set" class="mr-2 text-[var(--text-grey-color)]">UTXOs:</span>
          <span v-if="item?.utxo_set" class="mr-2 text-left">{{ item?.utxo_set?.length }}</span>

        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mt-auto">
        <Button
          second
          class="w-full px-0 flex items-center justify-center gap-2"
          @click="push('/')"
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

const loading = ref(true)
let connnectTry = 0
const LINKS = ref([])
let uaddrs = []
const zero = ref(false)
const teh = ref(false)
const techSkip = ['uns', 'intsk','intsk2', 'scrsk']


function goTo(address){
  push(`/utxos#${address}`)
}


async function updateBalances(){
  uaddrs = []
  const items = []
  const addrList = []
  const list = await sendMessage(GET_ADDRESSES, {refresh: true}, 'background')
  const utxoList = await sendMessage(GET_ALL_BALANCES, {}, 'background')

  for(let item of list?.addresses){
    if(!uaddrs.includes(item?.address)){
    if(teh.value === true){
      items.push(item)
    }else{
      if(!techSkip.includes(item.type)){
          items.push(item)
      }
    }
      uaddrs.push(item?.address)
    }
  }
  console.log('uaddrs:',uaddrs,' addrList:',items)

  if(utxoList?.length > 0 && items.length > 0){
  for(let i of items){
    for(let u of utxoList){
      if(i.address === u.address){i['utxo_set'] = u.utxo_set;}
    }
    if(zero.value === true){
        addrList.push(i);
    }else{
    if(i?.utxo_set){addrList.push(i);}
    }

  }
  console.log('addrList:',addrList)
  store.setAddresses(list?.addresses)
  LINKS.value = addrList
  loading.value = false
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
        }else{
          store.setAddresses(list?.addresses)
          LINKS.value = addrList
          loading.value = false
        }
      }
    }, 1000)
  }
}


onBeforeMount(async() => {

await updateBalances();


})

</script>

<style lang="scss" scoped>
.filters{
  align-self: flex-end;
}
.manage-addresses {
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
    align-items: center;
    text-align: center;
    letter-spacing: -0.32px;
    color: #1b46f5;
  }
}
</style>
