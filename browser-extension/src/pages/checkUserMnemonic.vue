<template>
  <div class="check-mnemonic-screen flex flex-col h-full" data-testid="check-mnemonic-page">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="check-mnemonic-screen_content overflow-y-auto overflow-x-hidden h-full flex flex-col items-start px-5 pb-5">
      <h1 class="text-[var(--text-color)]">Do you remember the mnemonic phrase?</h1>

      <div class="check-mnemonic-screen_text flex flex-col">
        <p class="mb-2 w-full text-left text-[var(--text-grey-color)]">
        Just to make sure: please fill in the missing words
        </p>
        <div
          class="check-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mt-2 mb-5"
        >
        <div
          v-if="flag_check"
          class="custom-input_error pb-1"
          :class="my-2"
        >
          <span class="text-red-300 text-left">{{errorMessage}}</span>
        </div>
        <NotifyInBody/>
        <table style="width: 100%;">
        <!-- for 12 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[0]" v-for="n in 4">
          <tr class="flex">
            <td class="w-full"><input :disabled="disable[n-1]" :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n-1]"/></td>
            <td class="w-full"><input :disabled="disable[n+3]" :placeholder="n+4" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+3]"/></td>
            <td class="w-full"><input :disabled="disable[n+7]" :placeholder="n+8" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+7]"/></td>
          </tr>
          </tbody>
          <!-- for 15 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[1]" v-for="n in 5">
          <tr class="flex">
            <td class="w-full"><input :disabled="disable[n-1]" :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n-1]"/></td>
            <td class="w-full"><input :disabled="disable[n+4]" :placeholder="n+5" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+4]"/></td>
            <td class="w-full"><input :disabled="disable[n+9]" :placeholder="n+10" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+9]"/></td>
          </tr>
          </tbody>

          <!-- for 18 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[2]" v-for="n in 6">
          <tr class="flex">
            <td class="w-full"><input :disabled="disable[n-1]" :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n-1]"/></td>
            <td class="w-full"><input :disabled="disable[n+5]" :placeholder="n+6" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+5]"/></td>
            <td class="w-full"><input :disabled="disable[n+11]" :placeholder="n+12" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+11]"/></td>
          </tr>
          </tbody>
          <!-- for 21 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[3]" v-for="n in 7">
          <tr class="flex">
            <td class="w-full"><input :disabled="disable[n-1]" :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n-1]"/></td>
            <td class="w-full"><input :disabled="disable[n+6]" :placeholder="n+7" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+6]"/></td>
            <td class="w-full"><input :disabled="disable[n+13]" :placeholder="n+14" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+13]"/></td>
          </tr>
          </tbody>
          <!-- for 24 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[4]" v-for="n in 8">
          <tr class="flex">
            <td class="w-full"><input :disabled="disable[n-1]" :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n-1]"/></td>
            <td class="w-full"><input :disabled="disable[n+7]" :placeholder="n+8" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+7]"/></td>
            <td class="w-full"><input :disabled="disable[n+15]" :placeholder="n+16" class="w-full bg-[var(--bg-color)] text-[var(--text-gray-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" data-testid="mnemonic-word" v-model="list[n+15]"/></td>
          </tr>
          </tbody>
        </table>
        </div>
        <Button
          v-show="checkEnvironment"
          class="w-full"
          @click="skip"
        >skip</Button>
      </div>
      <div class="flex w-full mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center"
          @click="goToBack"
          data-testid="go-back"
        >
          <ArrowLeftIcon />
        </Button>
        <Button
          enter
          class="w-full"
          @click="Check"
          :disabled="isDisabled"
          data-testid="check"
        >Check mnemonic phrase</Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import useWallet from '~/popup/modules/useWallet'
import { computed, ref, onBeforeMount } from 'vue'
import { useRouter } from 'vue-router'
import { SAVE_GENERATED_SEED, SET_UP_PASSWORD } from '~/config/events'
import {getRandom, saveGeneratedSeed, sendMessage} from '~/helpers/index'
import * as bip39 from "~/config/bip39";

const { back, push } = useRouter()
const { getFundAddress, getBalance, getNetWork } = useWallet()

const MNEMONIC_KEY = 'temp-mnemonic'
const MNEMONIC_LANGUAGE_KEY = 'temp-mnemonic-language'
const MNEMONIC_LENGTH_KEY = 'temp-mnemonic-length'
const PASSPHRASE_KEY = 'temp-passphrase'

const WORDS_COUNT = 'temp-words-count'
const ROW_POSITION= 'temp-row-position'

const flag_check = ref(false)
const mnemonicLength = ref(
  Number(localStorage?.getItem(MNEMONIC_LENGTH_KEY) || bip39.MNEMONIC_DEFAULT_LENGTH)
)

const errorMessage = ref('')

const us = ref(userChallenge())
const list = ref(us.value.list)
const disable = ref(us.value.dis)

const isDisabled = computed(() => {
  if(isEmpty()) return true
  return false
})

const checkEnvironment = ref(false);

function inputWords(e){
  if(isEmpty()) return false
}

function removeTempDataFromLocalStorage() {
  localStorage.removeItem(MNEMONIC_KEY)
  localStorage.removeItem(MNEMONIC_LENGTH_KEY)
  localStorage.removeItem(PASSPHRASE_KEY)
  localStorage.removeItem(MNEMONIC_LANGUAGE_KEY)
  localStorage.removeItem(WORDS_COUNT)
  localStorage.removeItem(ROW_POSITION)
}

function isEmpty(){
  for(const item of list.value){
    if(item === '') return true
  }
  return false
}

async function skip(){
  const tempMnemonic = localStorage?.getItem(MNEMONIC_KEY)
  const tempPassphrase = localStorage?.getItem(PASSPHRASE_KEY)
  const tempLanguage = localStorage?.getItem(MNEMONIC_LANGUAGE_KEY)
  const success = await saveGeneratedSeed(tempMnemonic, tempPassphrase, tempLanguage);
  if (success === true) {
    const fundAddress = await getFundAddress()
    getBalance(fundAddress)
    removeTempDataFromLocalStorage()
    return push('/wallet-created')
  }
}

function userChallenge(){
  const mnemonic = localStorage?.getItem(MNEMONIC_KEY)?.trim()?.split(' ') || []
  const dismap =  Array(mnemonic?.length).fill(true)
  const count = localStorage?.getItem(WORDS_COUNT) || getRandom(3,4)
  const row = localStorage?.getItem(ROW_POSITION)?.split(' ') || Array(count)
  for(let i = 0; i < count; i += 1){
    let index = row[i] || getRandom(1, mnemonic?.length -1)
    mnemonic[index] = ''
    dismap[index] = false
    row[i] = index
  }
  localStorage?.setItem(WORDS_COUNT, count)
  localStorage?.setItem(ROW_POSITION, row.join(' '))

  return {
  list: mnemonic,
  dis: dismap
  };
}

async function Check() {
  const tempMnemonic = localStorage?.getItem(MNEMONIC_KEY)
  const tempPassphrase = localStorage?.getItem(PASSPHRASE_KEY)
  const tempLanguage = localStorage?.getItem(MNEMONIC_LANGUAGE_KEY)
  const mnemonic = list.value.join(' ').trim()
  if (mnemonic === tempMnemonic) {
    const success = await saveGeneratedSeed(tempMnemonic, tempPassphrase, tempLanguage);
    if (success === true) {
      const fundAddress = await getFundAddress()
      getBalance(fundAddress)
      removeTempDataFromLocalStorage()
      return push('/wallet-created')
    }
  }
  flag_check.value = true
  errorMessage.value='The mnemonic phrase does not match the previously created one'
  setTimeout(()=>{
    flag_check.value = false
    errorMessage.value=''
  },3000)

}
function goToBack(){
  const isPassSetUpd = Boolean(localStorage?.getItem(SET_UP_PASSWORD))
  if(isPassSetUpd){
    return push('/generate')
  }
  return back()
}
onBeforeMount(async() => {
  const network = await getNetWork()
  checkEnvironment.value = (network !== ' ')
})
</script>

<style lang="scss" scoped>
.check-mnemonic-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;
    max-height: inherit;

    h1 {
      font-size: 18px;
      line-height: 25px;
      margin-bottom: 15px;
    }
  }

  &_text {
    p {
      font-weight: 400;
      font-size: 15px;
      line-height: 20px;
      letter-spacing: -0.32px;

      span {
        font-weight: 600;
      }
    }
  }
}
</style>
