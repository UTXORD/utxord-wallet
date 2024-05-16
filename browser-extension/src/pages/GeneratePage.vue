<template>
  <div class="generate-screen h-full flex flex-col">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="generate-screen_content h-full overflow-y-auto flex flex-col items-center px-5 pb-5"
    >
     <!-- title and content -->
      <h1 class="generate-screen_title w-full mb-2 text-left text-[var(--text-color)]">Your mnemonic phrase</h1>

      <!-- Secret phrase -->
      <div
        class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3"
      >
        <div class="flex flex-col items-center">
          <span class="w-full text-[var(--text-grey-color)] mb-2">Phrase’s length</span>
          <Dropdown
            :model-value="passphraseLength"
            @update:model-value="onChangePhraseLength"
            :options="PHRASE_LENGTH_OPTIONS"
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
          <span class="w-full text-[var(--text-grey-color)]">Show as: &nbsp;
            <input type="radio" v-model="picked" name="picked" value="line" />
            &nbsp;<label for="line">Line</label> or
            <input type="radio" v-model="picked" name="picked" value="list" checked />
            &nbsp;<label for="list">List</label>
          </span>
        </div>
        <CustomInput v-if="picked == 'line'"
          type="textarea"
          class="w-full"
          :rows="3"
          v-model="textarea"
          readonly
          :rules="[
          (val) => isASCII(val) || 'Please enter only Latin characters'
          ]"
        />
        <div
          v-if="!valid"
          class="custom-input_error"
          :class="my-2"
        >
          <span v-if="!valid" class="text-red-300 text-left">Invalid checksum menemonic</span>
        </div>
        <NotifyInBody/>
        <table style="width: 100%;" v-if="picked == 'list'">
          <!-- for 12 words -->
          <tbody v-if="passphraseLength == 12" v-for="n in 4">
            <tr>
              <td><input :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n-1]"/></td>
              <td><input :placeholder="n+4" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+3]"/></td>
              <td><input :placeholder="n+8" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+7]"/></td>
            </tr>
          </tbody>
          <!-- for 1s5 words -->
          <tbody v-if="passphraseLength == 15" v-for="n in 5">
            <tr>
              <td><input :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n-1]"/></td>
              <td><input :placeholder="n+5" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+4]"/></td>
              <td><input :placeholder="n+10" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+9]"/></td>
            </tr>
          </tbody>

          <!-- for 18 words -->
          <tbody v-if="passphraseLength == 18" v-for="n in 6">
            <tr>
              <td><input :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n-1]"/></td>
              <td><input :placeholder="n+6" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+5]"/></td>
              <td><input :placeholder="n+12" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+11]"/></td>
            </tr>
          </tbody>
          <!-- for 21 words -->
          <tbody v-if="passphraseLength == 21" v-for="n in 7">
            <tr>
              <td><input :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n-1]"/></td>
              <td><input :placeholder="n+7" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+6]"/></td>
              <td><input :placeholder="n+14" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+13]"/></td>
            </tr>
          </tbody>
          <!-- for 24 words -->
          <tbody v-if="passphraseLength == 24" v-for="n in 8">
            <tr>
              <td><input :placeholder="n" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n-1]"/></td>
              <td><input :placeholder="n+8" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+7]"/></td>
              <td><input :placeholder="n+16" class="bg-[var(--bg-color)] text-[var(--text-color)] noselect p-2.5 min-h-[33px]" size="10" type="text" :value="list[n+15]"/></td>
            </tr>
          </tbody>
        </table>
        <!-- Passphrase -->

        <!-- I saved my mnemonic -->
        <div class="w-full flex justify-start items-center gap-2 mt-2.5">
          <Checkbox v-model="usePassphrase">
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
        <Checkbox v-model="mnemonicIsSaved" class="mb-3">
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
        >
          <ArrowLeftIcon />
        </Button>
        <Button
          enter
          :disabled="isDisabled"
          class="w-full"
          @click="onStore"
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
import { sendMessage } from 'webext-bridge'
import { computed, ref, onBeforeMount } from 'vue'
import { useRouter } from 'vue-router'
import { SET_UP_PASSWORD } from '~/config/events'
import { isASCII, isLength, isContains, copyToClipboard, isMnemonicValid } from '~/helpers/index'
import NotifyInBody from '~/components/NotifyInBody.vue'

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

const MNEMONIC_KEY = 'temp-mnemonic'
const PASSPHRASE_LENGTH_KEY = 'temp-passphrase-length'
const PASSPHRASE_KEY = 'temp-passphrase'

const { back, push } = useRouter()
const textarea = ref('')
const usePassphrase = ref(
  Boolean(localStorage?.getItem(PASSPHRASE_KEY)) || false
)
const passphrase = ref('')
const passphraseLength = ref(
  Number(localStorage?.getItem(PASSPHRASE_LENGTH_KEY)) || LENGTH_12.value
)
const picked = ref('list')
const showInfo = ref(false)
const mnemonicIsSaved = ref(false)

const list = computed(() => textarea.value.split(' '))
const valid = ref(false)

const isDisabled = computed(() => {
  (async ()=>{
    valid.value = await isMnemonicValid(textarea.value.trim())
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
  localStorage.removeItem(PASSPHRASE_LENGTH_KEY)
  localStorage.removeItem(PASSPHRASE_KEY)
}

function saveTempDataToLocalStorage(){
  localStorage?.setItem(PASSPHRASE_LENGTH_KEY, passphraseLength.value)
  localStorage?.setItem(MNEMONIC_KEY, textarea.value)
  localStorage?.setItem(PASSPHRASE_KEY, passphrase.value)
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
  removeTempDataFromLocalStorage()
  getMnemonic()
}

function onChangePhraseLength(option) {
  passphraseLength.value = option;
  refreshMnemonic();
}

async function getMnemonic() {
  const tempMnemonic = localStorage?.getItem(MNEMONIC_KEY)
  const tempLength = localStorage?.getItem(PASSPHRASE_LENGTH_KEY)
  const tempPassphrase = localStorage?.getItem(PASSPHRASE_KEY)
  if (tempMnemonic) {
    textarea.value = tempMnemonic
    passphraseLength.value = tempLength
    passphrase.value = tempPassphrase
  } else {
    const mnemonic = await sendMessage('GENERATE_MNEMONIC', {
      length: passphraseLength.value
    }, 'background')
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
