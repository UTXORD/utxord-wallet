<template>
  <div class="generate-screen h-full flex flex-col">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="generate-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
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
          <span class="w-full text-[var(--text-grey-color)]"
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

        <div class="generate-screen_form-input flex flex-col p-3">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Use Passphrase:
            <input
              name="usePassphrase"
              type="checkbox"
              v-model="usePassphrase"
              />
            </span>
        </div>
        <div class="generate-screen_form-input flex flex-col" v-show="usePassphrase">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Passphrase(optional): </span
          >
          <CustomInput
            type="text"
            v-model="passphrase"
            />
        </div>
      </div>

      <!-- Inputs -->
      <div
        class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl px-3 pt-3 mb-5"
      >

        <div class="generate-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Enter your Password:</span
          >
          <CustomInput
            autofocus
            type="password"
            v-model="password"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters'
            ]"
          />
        </div>

        <div class="generate-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Confirm Password:</span
          >
          <CustomInput
            type="password"
            v-model="confirmPassword"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters',
              (val) =>
                val === password ||
                'Confirm Password does not match the Password'
            ]"
          />
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mb-5 mt-auto">
        <Button
          outline
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="goToStartPage"
        >
          <img src="/assets/arrow-left.svg" alt="Go Back" />
        </Button>
        <Button :disabled="isDisabled" class="w-full" @click="onStore"
          >Store</Button
        >
      </div>

      <!-- Info -->
      <p class="generate-screen_info text-[var(--text-blue)]">
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
import { isASCII, copyToClipboard } from '~/helpers/index'

const { push } = useRouter()
const { getFundAddress, getBalance } = useWallet()
const textarea = ref('')
const password = ref('')
const confirmPassword = ref('')
const usePassphrase = ref(false)
const passphrase = ref('')
const length = ref(12)
const picked = ref('line')

const list = computed(() =>textarea.value.split(' '))

const isDisabled = computed(() => {
  if (!textarea.value) return true
  if (!password.value.length || !confirmPassword.value.length) return true
  if (password.value !== confirmPassword.value) return true
  if (!isASCII(password.value) || !isASCII(confirmPassword.value)) return true
  return false
})

async function onStore() {
  const success = await sendMessage(
    SAVE_GENERATED_SEED,
    {
      seed: textarea.value,
      password: password.value,
      passphrase: passphrase.value,
    },
    'background'
  )
  if (success) {
    const fundAddress = await getFundAddress()
    getBalance(fundAddress)
    localStorage.removeItem('temp-mnemonic')
    localStorage.removeItem('temp-length')
    push('/')
  }
}

function goToStartPage() {
  localStorage.removeItem('temp-mnemonic')
  localStorage.removeItem('temp-length')
  push('/start')
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
  }
}
</style>
