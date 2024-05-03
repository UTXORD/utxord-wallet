<template>
  <div class="check-mnemonic-screen flex flex-col h-full">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="check-mnemonic-screen_content h-full flex flex-col items-start px-5">
      <h1 class="text-[var(--text-color)]">Do you remember the mnemonic phrase?</h1>

      <div class="check-mnemonic-screen_text flex flex-col">
        <p class="mb-2 w-full text-left text-[var(--text-grey-color)]">
        Please include some words from your mnemonic phrase
        </p>
        <div
          class="check-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mt-2 mb-3"
        >
        <div
          v-if="flag_check"
          class="custom-input_error"
          :class="my-2"
        >
          <span class="text-red-300 text-left">{{errorMessage}}</span>
        </div>
        <NotifyInBody/>
        <table style="width: 100%;">
        <!-- for 12 words -->
          <tbody v-if="passphraseLength == 12" v-for="n in 4">
          <tr>
            <td><input :disabled="disable[n-1]" :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]"/></td>
            <td><input :disabled="disable[n+3]" :placeholder="n+4" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+3]"/></td>
            <td><input :disabled="disable[n+7]" :placeholder="n+8" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+7]"/></td>
          </tr>
          </tbody>
          <!-- for 15 words -->
          <tbody v-if="passphraseLength == 15" v-for="n in 5">
          <tr>
            <td><input :disabled="disable[n-1]" :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]"/></td>
            <td><input :disabled="disable[n+4]" :placeholder="n+5" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+4]"/></td>
            <td><input :disabled="disable[n+9]" :placeholder="n+10" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+9]"/></td>
          </tr>
          </tbody>

          <!-- for 18 words -->
          <tbody v-if="passphraseLength == 18" v-for="n in 6">
          <tr>
            <td><input :disabled="disable[n-1]" :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]"/></td>
            <td><input :disabled="disable[n+5]" :placeholder="n+6" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+5]"/></td>
            <td><input :disabled="disable[n+11]" :placeholder="n+12" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+11]"/></td>
          </tr>
          </tbody>
          <!-- for 21 words -->
          <tbody v-if="passphraseLength == 21" v-for="n in 7">
          <tr>
            <td><input :disabled="disable[n-1]" :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]"/></td>
            <td><input :disabled="disable[n+6]" :placeholder="n+7" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+6]"/></td>
            <td><input :disabled="disable[n+13]" :placeholder="n+14" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+13]"/></td>
          </tr>
          </tbody>
          <!-- for 24 words -->
          <tbody v-if="passphraseLength == 24" v-for="n in 8">
          <tr>
            <td><input :disabled="disable[n-1]" :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]"/></td>
            <td><input :disabled="disable[n+7]" :placeholder="n+8" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+7]"/></td>
            <td><input :disabled="disable[n+15]" :placeholder="n+16" class="bg-[var(--bg-color)] text-[var(--text-gray-color)] px-3 py-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+15]"/></td>
          </tr>
          </tbody>
        </table>
        </div>

      </div>
      <div class="flex w-full mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center"
          @click="goToBack"
        >
          <ArrowLeftIcon />
        </Button>
        <Button
          enter
          class="w-full"
          @click="Check"
          :disabled="isDisabled"
        >Check mnemonic phrase</Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { sendMessage } from 'webext-bridge'
import useWallet from '~/popup/modules/useWallet'
import { computed, ref, onBeforeMount } from 'vue'
import { useRouter } from 'vue-router'
import { SAVE_GENERATED_SEED, SET_UP_PASSWORD } from '~/config/events'
import { isASCII, isLength, isMnemonicValid, getRandom} from '~/helpers/index'
const { back, push } = useRouter()

const { getFundAddress, getBalance } = useWallet()

const MNEMONIC_KEY = 'temp-mnemonic'
const PASSPHRASE_LENGTH_KEY = 'temp-passphrase-length'
const PASSPHRASE_KEY = 'temp-passphrase'

const WORDS_COUNT = 'temp-words-count'
const ROW_POSITION= 'temp-row-position'


const flag_check = ref(false)
const passphraseLength = ref(
  Number(localStorage?.getItem(PASSPHRASE_LENGTH_KEY)) || 12
)

const errorMessage = ref('')

const us = ref(userChallenge())
const list = ref(us.value.list)
const disable = ref(us.value.dis)

const isDisabled = computed(() => {
  if(isEmpty()) return true
  return false
})

function inputWords(e){
  if(isEmpty()) return false
  checkValid(list.value.join(' '))
}

function removeTempDataFromLocalStorage() {
  localStorage.removeItem(MNEMONIC_KEY)
  localStorage.removeItem(PASSPHRASE_LENGTH_KEY)
  localStorage.removeItem(PASSPHRASE_KEY)
  localStorage.removeItem(WORDS_COUNT)
  localStorage.removeItem(ROW_POSITION)
}

function isEmpty(){
  for(const item of list.value){
    if(item === '') return true
  }
  return false
}



function userChallenge(){
  const mnemonic = localStorage?.getItem(MNEMONIC_KEY)?.trim()?.split(' ') || []
  const dismap =  Array(mnemonic.length).fill(true)
  const count = localStorage?.getItem(WORDS_COUNT) || getRandom(3,4)
  const row = localStorage?.getItem(ROW_POSITION)?.split(' ') || Array(count)
  for(let i = 0; i <= count; i += 1){
    let index = row[i] || getRandom(1, mnemonic.length -1)
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
  const tempLength = localStorage?.getItem(PASSPHRASE_LENGTH_KEY)
  const tempPassphrase = localStorage?.getItem(PASSPHRASE_KEY)
  const mnemonic = list.value.join(' ').trim()
  if(mnemonic === tempMnemonic){
  const success = await sendMessage(
    SAVE_GENERATED_SEED,
    {
      seed: tempMnemonic,
      passphrase: tempPassphrase
    },
    'background')
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
    valid.value = true
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
</script>

<style lang="scss" scoped>
.check-mnemonic-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;

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
