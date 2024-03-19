<template>
  <div class="generate-screen h-full flex flex-col">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="generate-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
     <!-- title and content -->
      <p class="generate-screen_title text-[var(--text-color)]">Your mnemonic phrase</p>
      <!-- Secret phrase -->
      <div
        class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
      >
        <div class="flex items-center mb-2">
          <span class="w-full text-[var(--text-grey-color)]"
            >Words length:</span
            >
              <select
              @change="refreshMnemonic"
              v-model="length"
              style="font-size: 14px; font-family: Arial;"
              class="generate-screen_form w-full flex flex-col mr-2 bg-[var(--bg-color)] text-[var(--text-color)]">
                <option value="12" :selected="length === 12" class="w-full text-[var(--text-grey-color)]">12</option>
                <option value="15" :selected="length === 15" class="w-full text-[var(--text-grey-color)]">15</option>
                <option value="18" :selected="length === 18" class="w-full text-[var(--text-grey-color)]">18</option>
                <option value="21" :selected="length === 21" class="w-full text-[var(--text-grey-color)]">21</option>
                <option value="24" :selected="length === 24" class="w-full text-[var(--text-grey-color)]">24</option>
              </select>
            </div>
      </div>
    <div
        class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
    >
        <div class="flex items-center mb-2">
          <span class="w-full text-[var(--text-grey-color)]"
            >Store these safely:</span
          >
          <RefreshIconSeed @click="refreshMnemonic" class="cursor-pointer w-4 mr-2" />
          <CopyIcon
            class="cursor-pointer"
            @click="copyToClipboard(textarea, 'Mnemonic was copied!')"
          />
        </div>
        <div class="flex items-center mb-2">
          <span class="w-full text-[var(--text-grey-color)] hidden"
            >Show as: &nbsp;
          <input type="radio" v-model="picked" name="picked" value="line" checked/>
          &nbsp;<label for="line">Line</label> or
          <input type="radio" v-model="picked" name="picked" value="list" />
          &nbsp;<label for="list">List</label>
          </span>
          </div>
        <CustomInput v-if="picked == 'line'"
          type="textarea"
          class="w-full"
          rows="3"
          v-model="textarea"
          readonly
        />
        <table style="width: 100%;" v-if="picked == 'list'">
        <!-- for 12 words -->
        <tbody v-if="length == 12" v-for="n in 4">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n-1]"/></td>
          <td>{{n+4}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+3]"/></td>
          <td>{{n+8}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+7]"/></td>
        </tr>
        </tbody>
        <!-- for 15 words -->
        <tbody v-if="length == 15" v-for="n in 5">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n-1]"/></td>
          <td>{{n+5}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+4]"/></td>
          <td>{{n+10}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+9]"/></td>
        </tr>
        </tbody>

        <!-- for 18 words -->
        <tbody v-if="length == 18" v-for="n in 6">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n-1]"/></td>
          <td>{{n+6}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+5]"/></td>
          <td>{{n+12}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+11]"/></td>
        </tr>
        </tbody>
        <!-- for 21 words -->
        <tbody v-if="length == 21" v-for="n in 7">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n-1]"/></td>
          <td>{{n+7}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+6]"/></td>
          <td>{{n+14}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+13]"/></td>
        </tr>
        </tbody>
        <!-- for 24 words -->
        <tbody v-if="length == 24" v-for="n in 8">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n-1]"/></td>
          <td>{{n+8}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+7]"/></td>
          <td>{{n+16}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[n+15]"/></td>
        </tr>
        </tbody>
        </table>
        <!-- Passphrase -->

        <div class="generate-screen_form-input flex flex-col p-3 hidden">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Use Passphrase:
            <input
              name="usePassphrase"
              type="checkbox"
              v-model="usePassphrase"
              />
              <svg xmlns="http://www.w3.org/2000/svg" @click="viewShowInfo" x="0px" y="0px" width="16" height="16" viewBox="0 0 16 16"
              style="float: inline-end; margin-right: 45%; cursor: pointer;">
                <g id="help_passphrase">
                  <path fill-rule="evenodd" clip-rule="evenodd" d="M8 15C11.866 15 15 11.866 15 8C15 4.13401 11.866 1 8 1C4.13401 1 1 4.13401 1 8C1 11.866 4.13401 15 8 15ZM8.99915 8.98492C8.99863 8.97941 8.99825 8.97532 8.99929 8.97163C9.00229 8.96094 9.01723 8.95361 9.07542 8.92504C9.11276 8.90671 9.1679 8.87965 9.24911 8.83735C9.35159 8.78437 9.41371 8.75211 9.47222 8.72077C10.4683 8.18717 11 7.45921 11 6.14286C11 4.42193 9.67286 3 8 3C6.38377 3 5 4.0241 5 5.5C5 6.05228 5.44772 6.5 6 6.5C6.55228 6.5 7 6.05228 7 5.5C7 5.28145 7.38029 5 8 5C8.53628 5 9 5.49685 9 6.14286C9 6.64198 8.92458 6.74523 8.52778 6.95781C8.48255 6.98204 8.43413 7.00718 8.32524 7.06352C7.44966 7.51956 7 8.0135 7 9C7 9.55228 7.44772 10 8 10C8.55228 10 9 9.55228 9 9C9 8.99391 8.99954 8.98902 8.99915 8.98492ZM9 12C9 12.5523 8.55228 13 8 13C7.44772 13 7 12.5523 7 12C7 11.4477 7.44772 11 8 11C8.55228 11 9 11.4477 9 12Z"></path>
                </g>
              </svg>
              <div v-if="showInfo" class="generate-screen_help p-3">
              <!-- Close btn -->
              <button
                @click="viewShowInfo"
                class="top-[-2px] right-[-5px] px-4 py-3"
                style="margin-top: -1rem;margin-left: 13rem;padding-left: 0rem;">
                <svg
                  class="w-4 h-4 text-[var(--text-color)] fill-current"
                  role="button"
                  xmlns="http://www.w3.org/2000/svg"
                  viewBox="0 0 20 20"
                >
                  <title>Close</title>
                  <path
                    d="M14.348 14.849a1.2 1.2 0 0 1-1.697 0L10 11.819l-2.651 3.029a1.2 1.2 0 1 1-1.697-1.697l2.758-3.15-2.759-3.152a1.2 1.2 0 1 1 1.697-1.697L10 8.183l2.651-3.031a1.2 1.2 0 1 1 1.697 1.697l-2.758 3.152 2.758 3.15a1.2 1.2 0 0 1 0 1.698z"
                  />
                </svg>
              </button>
              <span class="w-full text-[var(--text-grey-color)]" style="position: relative;top: -1rem;">
                A passphrase is not a password! <br />
                Any variation entered in future loads a valid wallet, but with different addresses.<br />
                This feature provides optional added security for advanced users only.
              </span>
              </div>
            </span>
        </div>
        <div class="generate-screen_form-input flex flex-col" v-show="usePassphrase">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Passphrase(optional): </span
          >
          <CustomInput
            type="text"
            v-model="passphrase"
            name="passphrase"
            />
        </div>

      </div>
      <!-- Alert -->
      <div
          class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
      >
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >Do not share it with anyone </span
        >
      </div>
      <!-- I saved my mnemonic -->
      <div
          class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
      >
        <div class="generate-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-red)]"
            >I saved my mnemonic phrase</span
            >
            <input
              name="mnemonicIsSaved"
              type="checkbox"
              v-model="mnemonicIsSaved"
              />
          </div>
      </div>
      <!-- Buttons -->
      <div class="flex w-full mb-5 mt-auto">
        <Button
          outline
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="goToBack"
        >
          <img src="/assets/arrow-left.svg" alt="Go Back" />
        </Button>
        <Button :disabled="isDisabled" class="w-full" @click="onStore"
          >Confirm</Button
        >
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
import useWallet from '~/popup/modules/useWallet'
import { SAVE_GENERATED_SEED } from '~/config/events'
import { isASCII, isLength, isContains, copyToClipboard } from '~/helpers/index'

const { back, push } = useRouter()
const { getFundAddress, getBalance } = useWallet()
const textarea = ref('')
const usePassphrase = ref(false)
const passphrase = ref('')
const length = ref(12)
const picked = ref('line')
const showInfo = ref(false)
const mnemonicIsSaved = ref(false)

const list = computed(() =>textarea.value.split(' '))

const isDisabled = computed(() => {
  if (!mnemonicIsSaved.value) return true
  if (!textarea.value) return true
  return false
})

function viewShowInfo(){
  showInfo.value = !showInfo.value
  return true
}

async function onStore() {
  const success = await sendMessage(
    SAVE_GENERATED_SEED,
    {
      seed: textarea.value,
      passphrase: passphrase.value
    },
    'background'
  )
  if (success) {
    const fundAddress = await getFundAddress()
    getBalance(fundAddress)
    localStorage.removeItem('temp-mnemonic')
    localStorage.removeItem('temp-length')
    push('/loading#wallet-created')
  }
}

function goToBack() {
  localStorage.removeItem('temp-mnemonic')
  localStorage.removeItem('temp-length')
  back()
}

function refreshMnemonic() {
  localStorage.removeItem('temp-mnemonic')
  localStorage.removeItem('temp-length')
  getMnemonic()
}

async function getMnemonic() {
  const tempMnemonic = localStorage?.getItem('temp-mnemonic')
  const tempLength = localStorage?.getItem('temp-length')
  if (tempMnemonic) {
    textarea.value = tempMnemonic
    length.value = tempLength
  } else {
    const mnemonic = await sendMessage('GENERATE_MNEMONIC', {
      length: length.value
    }, 'background')
    localStorage?.setItem('temp-mnemonic', mnemonic)
    localStorage?.setItem('temp-length', length.value)
    textarea.value = mnemonic
  }
}

onBeforeMount(() => {
  getMnemonic()
})
</script>

<style lang="scss" scoped>
.generate-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;
  }

  &_title {
    font-size: 18px;
    line-height: 25px;
    margin-bottom: 15px;
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
