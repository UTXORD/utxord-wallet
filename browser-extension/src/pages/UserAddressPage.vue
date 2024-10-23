<template>
  <LoadingPage v-if="loading" />
  <div v-else class="user-address-screen flex flex-col h-full" data-testid="user-address-page">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="user-address-screen_content h-full flex flex-col items-start px-5">

      <h1 class="text-[var(--text-color)] text-[18px] mb-4">User Addresses Setup</h1>



      <div class="w-full flex flex-col bg-[var(--section)] rounded-xl px-3 py-3 mb-5">
        <span class="mb-2 w-full text-[var(--text-grey-color)] text-left text-[14px]">Manage custom derivation</span>


        <div
          class="sign-screen_block w-full flex flex-col bg-[var(--section)] rounded-lg p-3 mb-5"
        >
          <span class="mr-2 p-2 text-[var(--text-grey-color)]">Add your custom derivation path</span>
          <div class="user-address-screen_block-input flex flex-col">
            <CustomInput
              class="w-full"
              placeholder="m/86'/1'/2'/3/4"
              v-model="path"
            >
            </CustomInput>
            </div>
            <div class="user-address-screen_block-add-button flex flex-col p-2">
            <span class="mr-2 p-2 text-[var(--text-grey-color)]">{{checkMessage}}</span>
            <Button
              @click="onAdd"
              :disabled="isDisabled"
              enter
              class="w-full"
              data-testid="add-path"
            >
              Add
            </Button>
          </div>
        </div>

        <div class="flex flex-col bg-[var(--bg-color)] rounded-xl px-4">
          <div
            v-for="(p, i) in list"
            :key="i"
            class="flex justify-between items-center py-4 border-b-[1px] border-[var(--border-color)]"
          >
            {{p}}
            <Button
              @click="onDel(i)"
              data-testid="remove-path"
            >
              Delete
            </Button>
          </div>
          <div
          v-if="pMessage"
          class="flex justify-between items-center py-4 border-b-[1px] border-[var(--border-color)]">
            {{pMessage}}
          </div>
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mt-auto">
        <Button
          second
          class="w-full px-0 flex items-center justify-center gap-2"
          @click="back"
          data-testid="go-back"
        >
          Go Back
        </Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { sendMessage } from 'webext-bridge'
import { computed, ref, toRefs, onMounted} from 'vue'
import LoadingPage from '~/pages/LoadingPage.vue'
import { useRouter } from 'vue-router'
import { isASCII } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import {SAVE_USER_PATH, GET_USER_PATH, STATUS_DERIVATION} from '~/config/events';
import useWallet from '~/popup/modules/useWallet'

const TAPROOT_VALUE = 0
const SEGWIT_VALUE = 1


const store = useStore()
const { fundAddress, useDerivation, typeAddress } = toRefs(store)
const { getBalance, fetchUSDRate } = useWallet()
const { back, push } = useRouter()

const path = ref("m/86'/1'/2'/3/4")
const checkMessage = ref('')
const list = ref([])
const pMessage = computed(() =>(list.value.length===0)?'Path list is empty':'')

const isDisabled = computed(() => {
  if (path.value==='') return true
  return false
})

const loading = ref(true)

async function onDel(id:number){
  const newList = []
  for(const i in list.value){
    if(Number(i) !== id){
    newList.push(list.value[Number(i)])
    }
  }
  list.value = newList
  await sendMessage(SAVE_USER_PATH, {list: list.value}, 'background')
}

async function onAdd(){
  checkMessage.value = ''
  const check = /^m\/8[6,4]?\'\/?\d+?\'\/\d+?\'\/\d+?\/\d+$/.test(path.value)
  if(!check){
    checkMessage.value ='Invalid derivation path';
  return;
  }
  if(list.value.indexOf(path.value)!==-1){
    checkMessage.value ='Already present in the list';
    return;
  }
  list.value.push(path.value)
  path.value = ''
  await sendMessage(SAVE_USER_PATH, {list: list.value}, 'background')
}

function refreshBalance() {
  store.setSyncToFalse()
  fetchUSDRate()
  setTimeout(async () => {
    const res = await sendMessage(STATUS_DERIVATION, {}, 'background')
    await store.setUseDerivation(Boolean(res.derivate))
    const ta = Number(typeAddress.value);
    const tl = Boolean(useDerivation.value)?'fund':'oth'
    const addr = await res.keys?.addresses?.reverse()?.find(
      (item) => item.type === tl && item.typeAddress === ta
    )?.address
    await store.setFundAddress(addr)
    await getBalance(fundAddress.value);
  }, 1000)
}

onMounted(async () => {
try{
const res = await sendMessage(GET_USER_PATH, {}, 'background')
console.log('res:',res)
list.value = res || [];
}catch(e){
  console.log(e)
}

  setTimeout(() => {
    loading.value = false
  }, 1000)
})
</script>

<style lang="scss" scoped>
.user-address-screen {
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

  :deep(.question-icon) {
    path {
      fill: var(--text-grey-color);
    }

    &:hover {
      path {
        fill: var(--text-color);
      }
    }
  }
}
</style>
