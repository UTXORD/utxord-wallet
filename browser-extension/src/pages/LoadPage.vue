<template>
  <div class="load-screen flex flex-col h-full" data-testid="load-page">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="load-screen_content h-full overflow-y-auto overflow-x-hidden flex flex-col items-center px-5 pb-5">
      <!-- title and content -->
      <div class="flex flex-col w-full items-start mb-4">
        <h1 class="load-screen_title text-[var(--text-color)] mb-2">Import a mnemonic phrase</h1>
        <span class="w-full text-left text-[13px] text-[var(--text-grey-color)]">
          This is usually a set of 12–24 words
        </span>
      </div>

      <!-- Secret phrase -->
      <div
        class="load-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3"
      >
        <div class="flex items-center mb-2 hidden" data-testid="show-as">
          <span class="w-full text-[var(--text-grey-color)]"
            >Show as: &nbsp;
          <input type="radio" v-model="picked" name="picked" value="line" data-testid="picked-line"/>
          &nbsp;<label for="line">Line</label> or
          <input type="radio" v-model="picked" name="picked" value="list" data-testid="picked-list" checked />
          &nbsp;<label for="list">List</label>
          </span>
          </div>
          <div class="flex items-center mb-2" v-if="picked == 'list'">
            <span class="w-full text-[var(--text-grey-color)]"
              >How many words:</span
              >
              <Dropdown
                :model-value="passphraseLength"
                @update:model-value="onChangePhraseLength"
                :options="PHRASE_LENGTH_OPTIONS"
                data-testid="phrase-length"
              />
              </div>
        <CustomInput
          v-if="picked == 'line'"
          @change="inputWords"
          @input="inputWords"
          type="textarea"
          class="w-full"
          autofocus
          rows="3"
          placeholder="Enter your phrase here"
          v-model.lazy="textarea"
          :rules="[
            (val) => isASCII(val) || 'Please enter only Latin characters'
          ]"
          data-testid="mnemonic-phrase"
        />
        <div
          v-if="flag_check"
          class="custom-input_error"
          :class="my-2"
          data-testid="mnemonic-checksum-error"
        >
          <span v-if="!valid" class="text-red-300 text-left">Invalid checksum menemonic</span>
        </div>
        <NotifyInBody/>
        <table style="width: 100%;" v-if="picked == 'list'">
        <!-- for 12 words -->
          <tbody v-if="passphraseLength == 12" v-for="n in 4">
          <tr class="flex">
            <td class="w-full"><input :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+4" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]"  size="10" type="text" @input="inputWords" v-model="list[n+3]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+8" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]"  size="10" type="text" @input="inputWords" v-model="list[n+7]" data-testid="mnemonic-word"/></td>
          </tr>
          </tbody>
          <!-- for 15 words -->
          <tbody v-if="passphraseLength == 15" v-for="n in 5">
          <tr class="flex">
            <td class="w-full"><input :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+5" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+4]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+10" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+9]" data-testid="mnemonic-word"/></td>
          </tr>
          </tbody>

          <!-- for 18 words -->
          <tbody v-if="passphraseLength == 18" v-for="n in 6">
          <tr class="flex">
            <td class="w-full"><input :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+6" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+5]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+12" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+11]" data-testid="mnemonic-word"/></td>
          </tr>
          </tbody>
          <!-- for 21 words -->
          <tbody v-if="passphraseLength == 21" v-for="n in 7">
          <tr class="flex">
            <td class="w-full"><input :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+7" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+6]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+14" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+13]" data-testid="mnemonic-word"/></td>
          </tr>
          </tbody>
          <!-- for 24 words -->
          <tbody v-if="passphraseLength == 24" v-for="n in 8">
          <tr class="flex">
            <td class="w-full"><input :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n-1]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+8" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+7]" data-testid="mnemonic-word"/></td>
            <td class="w-full"><input :placeholder="n+16" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] pl-2.5 min-h-[33px]" size="10" type="text" @input="inputWords" v-model="list[n+15]" data-testid="mnemonic-word"/></td>
          </tr>
          </tbody>
        </table>
      </div>
      <!-- Passphrase -->
      <hr class="w-full border-[var(--border-color)] my-5" />

      <!-- I saved my mnemonic -->
      <div class="w-full flex justify-start items-center gap-2 mb-5">
        <Checkbox v-model="usePassphrase" data-testid="use-passphrase">
          <span class="w-full text-[var(--text-grey-color)] text-[15px]">
            Use Passphrase
          </span>
        </Checkbox>

        <VDropdown :triggers="['hover']" :distance="10" placement="top">
          <QuestionIcon class="question-icon cursor-pointer" />
          <template #popper>
            <div class="max-w-[250px] load-screen_tooltip-descr bg-black py-2 px-3 text-white">
              A passphrase is not a password; it behaves differently. It will be accepted if you already have a passphrase and enter it wrong. However, it will produce an empty new set of addresses.
            </div>
          </template>
        </VDropdown>
      </div>

      <!-- Password field -->
      <div
        class="load-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
        v-show="usePassphrase"
      >
        <div class="load-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]">Passphrase (optional)</span>
          <CustomInput
            type="password"
            v-model="passphrase"
            name="passphrase"
            @input="saveTempDataToLocalStorage"
            data-testid="passphrase"
          />
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center"
          @click="goToBack"
          data-testid="go-back"
        >
          <ArrowLeftIcon />
        </Button>
        <Button @click="onStore" :disabled="isDisabled" class="w-full" data-testid="confirm">Confirm</Button>
      </div>

      <!-- Info -->
      <p class="hidden load-screen_info text-[var(--text-blue)]">
        The data is stored locally in this extension
      </p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { sendMessage } from 'webext-bridge'
import { useRouter } from 'vue-router'
import { SAVE_GENERATED_SEED, SET_UP_PASSWORD } from '~/config/events'
import { isASCII, isLength, isContains, copyToClipboard, isMnemonicValid} from '~/helpers/index'
import useWallet from '~/popup/modules/useWallet'
import NotifyInBody from '~/components/NotifyInBody.vue'

const { back, push } = useRouter()

const { getFundAddress, getBalance } = useWallet()

const LENGTH_12 = { label: '12', value: 12 }
const LENGTH_15 = { label: '15', value: 15 }
const LENGTH_18 = { label: '18', value: 18 }
const LENGTH_21 = { label: '21', value: 21 }
const LENGTH_24 = { label: '24', value: 24 }

const PHRASE_LENGTH_OPTIONS = [
  LENGTH_12,
  LENGTH_15,
  LENGTH_18,
  LENGTH_21,
  LENGTH_24
]


const textarea = ref('')

const picked = ref('list')

const usePassphrase = ref(
  Boolean(localStorage?.getItem(PASSPHRASE_KEY)) || false
)

const passphrase = ref(
  localStorage?.getItem(PASSPHRASE_KEY) || ''
)
const list = ref(
  localStorage?.getItem(MNEMONIC_KEY)?.trim()?.split(' ') || []
)
const passphraseLength = ref(
  Number(localStorage?.getItem(MNEMONIC_LENGTH)) || LENGTH_12.value
)
const showInfo = ref(false)

const valid = ref(false)

const flag_check = ref(false)

const MNEMONIC_KEY = 'temp-mnemonic'
const MNEMONIC_LENGTH = 'mnemonic-length'
const PASSPHRASE_KEY = 'passphrase-key'

function onChangePhraseLength(option) {
  passphraseLength.value = option;
  saveTempDataToLocalStorage()
}

const isDisabled = computed(() => {
  checkValid(textarea.value?.trim())
  const seedArr = textarea.value?.trim()?.split(' ')
  if (seedArr.length !== 12 &&
      seedArr.length !== 15 &&
      seedArr.length !== 18 &&
      seedArr.length !== 21 &&
      seedArr.length !== 24) return true
  if (!textarea.value) return true
  if (!valid.value) return true
  return false
})

function checkValid(val = ''){
  (async ()=>{
    valid.value = await isMnemonicValid(val.trim())
    if(val.length){
     flag_check.value = true
     }else{
     flag_check.value = false
     }
  })()
  return valid.value
}

async function inputWords(e){
console.log('run inputWords');
    checkValid(textarea.value?.trim())
  const l = await list.value
console.log('picked.value:', picked.value);
  if(picked.value === 'line'){
    list.value = textarea.value?.trim()?.split(' ')
    if (list.value.length === 12 ||
        list.value.length === 15 ||
        list.value.length === 18 ||
        list.value.length === 21 ||
        list.value.length === 24){
          passphraseLength.value = list.value.length
          saveTempDataToLocalStorage()
          return;
        }
        passphraseLength.value = 12
        saveTempDataToLocalStorage()
    return;
  }

  if(picked.value === 'list'){
    if(e.target.value?.trim()?.split(' ')?.length > list.value.length){
      textarea.value = e.target.value
      list.value = textarea.value?.trim()?.split(' ')
    }
    if(list.value.length >= e.target.value?.trim()?.split(' ')?.length){
      textarea.value = list.value?.join(' ')?.trim()
    }

    if (list.value.length === 12 ||
        list.value.length === 15 ||
        list.value.length === 18 ||
        list.value.length === 21 ||
        list.value.length === 24){
        passphraseLength.value = list.value.length
        saveTempDataToLocalStorage()
          return;
    }
    saveTempDataToLocalStorage()
    return;
  }
  if(!passphraseLength.value) passphraseLength.value = 12
  saveTempDataToLocalStorage()
  return;
}

function saveTempDataToLocalStorage(){
  localStorage?.setItem(MNEMONIC_LENGTH, passphraseLength.value)
  localStorage?.setItem(MNEMONIC_KEY, list.value.join(' '))
  localStorage?.setItem(PASSPHRASE_KEY, passphrase.value)
}

function removeTempDataFromLocalStorage() {
  localStorage?.removeItem(MNEMONIC_LENGTH)
  localStorage?.removeItem(MNEMONIC_KEY)
  localStorage?.removeItem(PASSPHRASE_KEY)
}

function viewShowInfo(){
  showInfo.value = !showInfo.value
  return true
}

async function onStore() {
  localStorage?.setItem(MNEMONIC_KEY, textarea.value.replace(/\s\s+/g, ' ').trim())
  const generated = await sendMessage(
    SAVE_GENERATED_SEED,
    {
      seed: textarea.value.replace(/\s\s+/g, ' ').trim(),
      passphrase: passphrase.value
    },
    'background'
  )
  if (generated === true) {
    await getFundAddress()
    getBalance()
    removeTempDataFromLocalStorage()
    push('/')
  }
}

function goToBack(){
  const isPassSetUpd = Boolean(localStorage?.getItem(SET_UP_PASSWORD))
  removeTempDataFromLocalStorage()
  if(isPassSetUpd){
    return push('/start')
  }
  return back()
}

onBeforeMount(() => {
  console.log('onBeforeMount')
  if(list.value.length > 0) inputWords({
    target:{
      value: list.value.join(' ')
    }
  })

})
</script>

<style lang="scss" scoped>
.load-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;
    max-height: inherit;
  }

  &_title {
    font-size: 18px;
    line-height: 25px;
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
    /* display: flex; */
    align-items: center;
    text-align: center;
    letter-spacing: -0.32px;
  }
  &_help {
    box-sizing: border-box;
    border-width: thin;
    border-style: solid;
    border-color: white;
    border-radius: 0.75rem;
    height: 10em;
  }

  &_tooltip-descr {
    font-weight: 400;
    font-size: 14px;
    line-height: 22px;
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
