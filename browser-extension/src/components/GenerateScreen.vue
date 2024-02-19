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
        <tbody v-if="length == 12">
        <tr>
          <td>1.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[0]"/></td>
          <td>5.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[4]"/></td>
          <td>9.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[8]"/></td>
        </tr>
        <tr>
          <td>2.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[1]"/></td>
          <td>6.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[5]"/></td>
          <td>10.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[9]"/></td>
        </tr>
        <tr>
          <td>3.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[2]"/></td>
          <td>7.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[6]"/></td>
          <td>11.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[10]"/></td>
        </tr>
        <tr>
          <td>4.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[4]"/></td>
          <td>8.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[7]"/></td>
          <td>12.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[11]"/></td>
        </tr>
        </tbody>
        <!-- for 15 words -->
        <tbody v-if="length == 15">
        <tr>
          <td>1.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[0]"/></td>
          <td>6.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[5]"/></td>
          <td>11.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[10]"/></td>
        </tr>
        <tr>
          <td>2.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[1]"/></td>
          <td>7.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[6]"/></td>
          <td>12.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[11]"/></td>
        </tr>
        <tr>
          <td>3.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[2]"/></td>
          <td>8.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[7]"/></td>
          <td>13.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[12]"/></td>
        </tr>
        <tr>
          <td>4.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[3]"/></td>
          <td>9.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[8]"/></td>
          <td>14.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[13]"/></td>
        </tr>
        <tr>
          <td>5.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[5]"/></td>
          <td>10.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[9]"/></td>
          <td>15.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[14]"/></td>
        </tr>
        </tbody>

        <!-- for 18 words -->
        <tbody v-if="length == 18">
        <tr>
          <td>1.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[0]"/></td>
          <td>7.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[6]"/></td>
          <td>13.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[12]"/></td>
        </tr>
        <tr>
          <td>2.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[2]"/></td>
          <td>8.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[7]"/></td>
          <td>14.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[13]"/></td>
        </tr>
        <tr>
          <td>3.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[2]"/></td>
          <td>9.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[8]"/></td>
          <td>15.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[14]"/></td>
        </tr>
        <tr>
          <td>4.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[3]"/></td>
          <td>10.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[9]"/></td>
          <td>16.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[15]"/></td>
        </tr>
        <tr>
          <td>5.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[4]"/></td>
          <td>11.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[10]"/></td>
          <td>17.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[16]"/></td>
        </tr>
        <tr>
          <td>6.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[5]"/></td>
          <td>12.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[11]"/></td>
          <td>18.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[17]"/></td>
        </tr>
        </tbody>
        <!-- for 21 words -->
        <tbody v-if="length == 21">
        <tr>
          <td>1.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[0]"/></td>
          <td>8.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[7]"/></td>
          <td>15.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[14]"/></td>
        </tr>
        <tr>
          <td>2.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[1]"/></td>
          <td>9.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[8]"/></td>
          <td>16.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[15]"/></td>
        </tr>
        <tr>
          <td>3.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[2]"/></td>
          <td>10.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[9]"/></td>
          <td>17.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[16]"/></td>
        </tr>
        <tr>
          <td>4.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[3]"/></td>
          <td>11.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[10]"/></td>
          <td>18.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[17]"/></td>
        </tr>
        <tr>
          <td>5.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[4]"/></td>
          <td>12.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[11]"/></td>
          <td>19.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[18]"/></td>
        </tr>
        <tr>
          <td>6.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[5]"/></td>
          <td>13.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[12]"/></td>
          <td>20.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[19]"/></td>
        </tr>
        <tr>
          <td>7.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[6]"/></td>
          <td>14.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[13]"/></td>
          <td>21.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[20]"/></td>
        </tr>
        </tbody>
        <!-- for 24 words -->
        <tbody v-if="length == 24">
        <tr>
          <td>1.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[0]"/></td>
          <td>9.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[8]"/></td>
          <td>17.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[16]"/></td>
        </tr>
        <tr>
          <td>2.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[1]"/></td>
          <td>10.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[9]"/></td>
          <td>18.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[17]"/></td>
        </tr>
        <tr>
          <td>3.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[2]"/></td>
          <td>11.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[10]"/></td>
          <td>19.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[18]"/></td>
        </tr>
        <tr>
          <td>4.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[3]"/></td>
          <td>12.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[11]"/></td>
          <td>20.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[19]"/></td>
        </tr>
        <tr>
          <td>5.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[4]"/></td>
          <td>13.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[12]"/></td>
          <td>21.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[20]"/></td>
        </tr>
        <tr>
          <td>6.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[5]"/></td>
          <td>14.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[13]"/></td>
          <td>22.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[21]"/></td>
        </tr>
        <tr>
          <td>7.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[6]"/></td>
          <td>15.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[14]"/></td>
          <td>23.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[22]"/></td>
        </tr>
        <tr>
          <td>8.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[7]"/></td>
          <td>16.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[15]"/></td>
          <td>24.<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" :value="list[23]"/></td>
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
