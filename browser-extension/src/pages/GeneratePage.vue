<template>
  <div class="generate-screen h-full flex flex-col" data-testid="generate-page">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="generate-screen_content h-full overflow-y-auto overflow-x-hidden flex flex-col items-center px-5 pb-5"
    >
     <!-- title and content -->
      <h1 class="generate-screen_title w-full mb-2 text-left text-[var(--text-color)]">Your mnemonic phrase</h1>

      <!-- Secret phrase -->
      <div
        class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3"
      >

        <div class="flex flex-col items-center">
          <span class="w-full text-[var(--text-grey-color)] mb-2">Phrase’s language:</span>
          <Dropdown
            :model-value="mnemonicLanguage"
            @update:model-value="onChangeMnemonicLanguage"
            :options="bip39.MNEMONIC_LANGUAGE_OPTIONS"
            data-testid="phrase-language"
          />
        </div>

        <div class="flex flex-col items-center">
          <span class="w-full text-[var(--text-grey-color)] mb-2">Phrase’s length:</span>
          <Dropdown
            :model-value="mnemonicLength"
            @update:model-value="onChangeMnemonicLength"
            :options="bip39.MNEMONIC_LENGTH_OPTIONS"
            data-testid="mnemonic-length"
          />
        </div>

      </div>

      <div
        class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mt-4"
      >
        <div class="flex items-center mb-1">
          <span class="w-full text-[var(--text-grey-color)]"
            >Store these safely:</span
          >
          <RefreshIconSeed @click="refreshMnemonic" class="cursor-pointer w-4 mr-2" />
          <CopyIcon
            class="cursor-pointer"
            @click="copyToClipboard(textarea, 'Mnemonic was copied!')"
          />
        </div>
        <div class="flex items-center mb-2 hidden">
          <span class="w-full text-[var(--text-grey-color)]" data-testid="show-as">Show as: &nbsp;
            <input type="radio" v-model="picked" name="picked" value="line" data-testid="picked-line" />
            &nbsp;<label for="line">Line</label> or
            <input type="radio" v-model="picked" name="picked" value="list" data-testid="picked-list" checked />
            &nbsp;<label for="list">List</label>
          </span>
        </div>
        <CustomInput v-if="picked == 'line'"
          type="textarea"
          class="w-full"
          :rows="3"
          v-model="textarea"
          readonly="readonly"
          :rules="[
          (val) => isASCII(val) || 'Please enter only Latin characters'
          ]"
          data-testid="mnemonic-phrase"
        />
        <div
          v-if="!valid"
          class="custom-input_error"
          :class="my-2"
          data-testid="mnemonic-checksum-error"
        >
          <span v-if="!valid" class="text-red-300 text-left">Invalid checksum mnemonic</span>
        </div>
        <NotifyInBody/>
        <table style="width: 100%;" v-if="picked == 'list'">
          <!-- for 12 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[0]" v-for=" n in 4">
            <tr class="flex">
              <td class="w-full"><input :placeholder="n" class="w-full  bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n-1]"/></td>
              <td class="w-full"><input :placeholder="n+4" class="w-full  bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+3]"/></td>
              <td class="w-full"><input :placeholder="n+8" class="w-full  bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+7]"/></td>
            </tr>
          </tbody>
          <!-- for 15 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[1]" v-for="n in 5">
            <tr class="flex">
              <td class="w-full"><input :placeholder="n" class="w-full  bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n-1]"/></td>
              <td class="w-full"><input :placeholder="n+5" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+4]"/></td>
              <td class="w-full"><input :placeholder="n+10" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+9]"/></td>
            </tr>
          </tbody>
          <!-- for 18 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[2]" v-for="n in 6">
            <tr class="flex">
              <td class="w-full"><input :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n-1]"/></td>
              <td class="w-full"><input :placeholder="n+6" class="w-full  bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+5]"/></td>
              <td class="w-full"><input :placeholder="n+12" class="w-full  bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+11]"/></td>
            </tr>
          </tbody>
          <!-- for 21 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[3]" v-for="n in 7">
            <tr class="flex">
              <td class="w-full"><input :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n-1]"/></td>
              <td class="w-full"><input :placeholder="n+7" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+6]"/></td>
              <td class="w-full"><input :placeholder="n+14" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+13]"/></td>
            </tr>
          </tbody>
          <!-- for 24 words -->
          <tbody v-if="mnemonicLength == bip39.MNEMONIC_LENGTHS[4]" v-for="n in 8">
            <tr class="flex">
              <td class="w-full"><input :placeholder="n" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n-1]"/></td>
              <td class="w-full"><input :placeholder="n+8" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+7]"/></td>
              <td class="w-full"><input :placeholder="n+16" class="w-full bg-[var(--bg-color)] text-[var(--text-color)] noselect pl-2.5 min-h-[33px]" size="10" type="text" data-testid="mnemonic-word" :value="list[n+15]"/></td>
            </tr>
          </tbody>
        </table>
        <!-- Passphrase -->

        <!-- I saved my mnemonic -->
        <div class="w-full flex justify-start items-center gap-2 mt-2.5">
          <Checkbox v-model="usePassphrase" data-testid="use-passphrase">
            <span class="w-full text-[var(--text-grey-color)] text-[15px]">
              Use Passphrase
            </span>
          </Checkbox>
          <VDropdown :triggers="['hover']" :distance="10" placement="top">
            <QuestionIcon class="question-icon cursor-pointer" />
            <template #popper>
              <div class="max-w-[250px] generate-screen_tooltip-descr bg-black py-2 px-3 text-white">
                A passphrase is not a password; it behaves differently. It will be accepted if you already have a passphrase and enter it wrong. However, it will produce an empty new set of addresses.
              </div>
            </template>
          </VDropdown>
        </div>

        <!-- Password field -->
        <div
          class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl pt-3 mt-1"
          v-show="usePassphrase"
        >
          <div class="generate-screen_form-input flex flex-col">
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
        </div>

      <!-- Alert -->
      <div class="w-full flex justify-start mt-3">
        <AlertBox>
          <span class="w-full text-[15px]">
            Do not share it with anyone
          </span>
        </AlertBox>
      </div>

      <hr class="w-full border-[var(--border-color)] my-4" />

      <!-- I saved my mnemonic -->
      <div class="w-full flex justify-start">
        <Checkbox v-model="mnemonicIsSaved" class="mb-3" data-testid="i-saved-my-mnemonic">
          <span class="w-full text-[var(--text-grey-color)] text-[15px]">
            I saved my mnemonic phrase
          </span>
        </Checkbox>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mt-6">
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
          :disabled="isDisabled"
          class="w-full"
          @click="onStore"
          data-testid="confirm"
        >Confirm</Button>
      </div>

      <!-- Info -->
      <p class="hidden generate-screen_info text-[var(--text-blue)]" >
        The data is stored locally in this extension
      </p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, onBeforeMount } from 'vue'
import { useRouter } from 'vue-router'
import {
  isASCII,
  copyToClipboard,
  generateMnemonic,
  isMnemonicValid,
  } from '~/helpers/index'
import NotifyInBody from '~/components/NotifyInBody.vue'
import * as bip39 from "~/config/bip39";

const MNEMONIC_KEY = 'temp-mnemonic'
const MNEMONIC_LANGUAGE_KEY = 'temp-mnemonic-language'
const MNEMONIC_LENGTH_KEY = 'temp-mnemonic-length'
const PASSPHRASE_KEY = 'temp-passphrase'

const WORDS_COUNT = 'temp-words-count'
const ROW_POSITION= 'temp-row-position'

const { back, push } = useRouter()

const textarea = ref('')
const usePassphrase = ref(
  Boolean(localStorage?.getItem(PASSPHRASE_KEY)) || false
)
const passphrase = ref('')
const mnemonicLength = ref(
  Number(localStorage?.getItem(MNEMONIC_LENGTH_KEY) || bip39.MNEMONIC_DEFAULT_LENGTH)
)
const mnemonicLanguage = ref(
  String(localStorage?.getItem(MNEMONIC_LANGUAGE_KEY) || bip39.MNEMONIC_DEFAULT_LANGUAGE)
)

const picked = ref('list')
const showInfo = ref(false)
const mnemonicIsSaved = ref(false)

const list = computed(() => textarea.value.split(' '))
const valid = ref(false)

const isDisabled = computed(() => {
  (async ()=>{
    valid.value = await isMnemonicValid(textarea.value.trim(), mnemonicLanguage.value);
  })()
  if (!textarea.value) return true
  if (!valid.value) return true
  if (!mnemonicIsSaved.value) return true
  return false
})

function viewShowInfo() {
  showInfo.value = !showInfo.value
  return true
}

function removeTempDataFromLocalStorage() {
  localStorage.removeItem(MNEMONIC_KEY)
  localStorage.removeItem(MNEMONIC_LENGTH_KEY)
  localStorage.removeItem(PASSPHRASE_KEY)
  localStorage.removeItem(MNEMONIC_LANGUAGE_KEY)
}

function saveTempDataToLocalStorage(){
  localStorage?.setItem(MNEMONIC_LENGTH_KEY, mnemonicLength.value)
  localStorage?.setItem(MNEMONIC_KEY, textarea.value)
  localStorage?.setItem(PASSPHRASE_KEY, passphrase.value)
  localStorage?.setItem(MNEMONIC_LANGUAGE_KEY, mnemonicLanguage.value)
  localStorage?.removeItem(WORDS_COUNT)
  localStorage?.removeItem(ROW_POSITION)
}

async function onStore() {
    saveTempDataToLocalStorage()
    push('/check-user-mnemonic')
}

function goToBack() {
  removeTempDataFromLocalStorage()
  return push('/start')
}

function refreshMnemonic() {
  removeTempDataFromLocalStorage();
  getMnemonic()
}

function onChangeMnemonicLength(option) {
  mnemonicLength.value = option;
  saveTempDataToLocalStorage();
  refreshMnemonic();
}

function onChangeMnemonicLanguage(option) {
  mnemonicLanguage.value = option;
  saveTempDataToLocalStorage();
  refreshMnemonic();
}

async function getMnemonic() {
  const tempMnemonic = localStorage?.getItem(MNEMONIC_KEY)
  const tempLength = localStorage?.getItem(MNEMONIC_LENGTH_KEY)
  const tempPassphrase = localStorage?.getItem(PASSPHRASE_KEY)
  const tempLanguage = localStorage?.getItem(MNEMONIC_LANGUAGE_KEY)
  if (tempMnemonic) {
    console.debug('=== tempMnemonic, tempLanguage:', tempLanguage);
    textarea.value = tempMnemonic
    mnemonicLength.value = tempLength
    passphrase.value = tempPassphrase
    mnemonicLanguage.value = tempLanguage
  } else {
    console.debug('=== !tempMnemonic');
    const mnemonic = await generateMnemonic(mnemonicLength.value, mnemonicLanguage.value);
    textarea.value = mnemonic
    saveTempDataToLocalStorage()
  }
}

onBeforeMount(() => {
  getMnemonic()
})
</script>

<style lang="scss" scoped>

.noselect {
  -moz-user-select: none;
  -webkit-user-select: none;
  -ms-user-select: none;
  -o-user-select: none;
  user-select: none;
}

.generate-screen {
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
}
</style>
