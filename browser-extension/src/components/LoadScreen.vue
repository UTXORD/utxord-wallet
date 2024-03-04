<template>
  <div class="load-screen flex flex-col h-full">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="load-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <!-- Secret phrase -->
      <div
        class="load-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
      >
        <span class="mb-2 w-full text-[var(--text-grey-color)]"
          >Paste your secret words here:</span
        >
        <div class="flex items-center mb-2">
          <span class="w-full text-[var(--text-grey-color)]"
            >Show as: &nbsp;
          <input type="radio" v-model="picked" name="picked" value="line" checked/>
          &nbsp;<label for="line">Line</label> or
          <input type="radio" v-model="picked" name="picked" value="list" />
          &nbsp;<label for="list">List</label>
          </span>
          </div>
          <div class="flex items-center mb-2" v-if="picked == 'list'">
            <span class="w-full text-[var(--text-grey-color)]"
              >Words length:</span
              >
                <select
                v-model="length"
                style="font-size: 14px; font-family: Arial;"
                class="load-screen_form w-full flex flex-col mr-2 bg-[var(--bg-color)] text-[var(--text-color)]">
                  <option value="12" :selected="length === 12" class="w-full text-[var(--text-grey-color)]">12</option>
                  <option value="15" :selected="length === 15" class="w-full text-[var(--text-grey-color)]">15</option>
                  <option value="18" :selected="length === 18" class="w-full text-[var(--text-grey-color)]">18</option>
                  <option value="21" :selected="length === 21" class="w-full text-[var(--text-grey-color)]">21</option>
                  <option value="24" :selected="length === 24" class="w-full text-[var(--text-grey-color)]">24</option>
                </select>
              </div>
        <CustomInput v-if="picked == 'line'"
          @change="inputWords"
          type="textarea"
          class="w-full"
          autofocus
          rows="3"
          v-model="textarea"
        />
        <table style="width: 100%;" v-if="picked == 'list'">
        <!-- for 12 words -->
        <tbody v-if="length == 12" v-for="n in 4">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @input="inputWords" v-model="list[n-1]"/></td>
          <td>{{n+4}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @input="inputWords" v-model="list[n+3]"/></td>
          <td>{{n+8}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @input="inputWords" v-model="list[n+7]"/></td>
        </tr>
        </tbody>
        <!-- for 15 words -->
        <tbody v-if="length == 15" v-for="n in 5">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n-1]"/></td>
          <td>{{n+5}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+4]"/></td>
          <td>{{n+10}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+9]"/></td>
        </tr>
        </tbody>

        <!-- for 18 words -->
        <tbody v-if="length == 18" v-for="n in 6">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n-1]"/></td>
          <td>{{n+6}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+5]"/></td>
          <td>{{n+12}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+11]"/></td>
        </tr>
        </tbody>
        <!-- for 21 words -->
        <tbody v-if="length == 21" v-for="n in 7">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n-1]"/></td>
          <td>{{n+7}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+6]"/></td>
          <td>{{n+14}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+13]"/></td>
        </tr>
        </tbody>
        <!-- for 24 words -->
        <tbody v-if="length == 24" v-for="n in 8">
        <tr>
          <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n-1]"/></td>
          <td>{{n+8}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+7]"/></td>
          <td>{{n+16}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+15]"/></td>
        </tr>
        </tbody>
        </table>
        <!-- Passphrase -->

        <div class="load-screen_form-input flex flex-col p-3">
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
              <div v-if="showInfo" class="load-screen_help p-3">
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
        <div class="load-screen_form-input flex flex-col" v-show="usePassphrase">
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
        class="load-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl px-3 pt-3 mb-5"
      >
        <div class="load-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]"
            >Enter your Password:</span
          >
          <CustomInput
            type="password"
            v-model="password"
            :rules="[
              (val) => isASCII(val) || 'Please enter only Latin characters',
              (val) => isLength(val) || 'Password must be minimum 9 characters',
              (val) => isContains(val) || 'Password contains atleast One Uppercase, One Lowercase, One Number and One Special Chacter'
            ]"
          />
        </div>
        <div class="load-screen_form-input flex flex-col">
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
                'Confirm Password does not match the Password',
              (val) => isLength(val) || 'Password must be minimum 9 characters',
              (val) => isContains(val) ||
                'Password contains atleast One Uppercase, One Lowercase, One Number and One Special Chacter'

            ]"
          />
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mb-5 mt-auto">
        <Button
          outline
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="back"
        >
          <img src="/assets/arrow-left.svg" alt="Go Back" />
        </Button>
        <Button @click="onStore" :disabled="isDisabled" class="w-full"
          >Load</Button
        >
      </div>

      <!-- Info -->
      <p class="load-screen_info text-[var(--text-blue)]">
        The data is stored locally in this extension
      </p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { sendMessage } from 'webext-bridge'
import { useRouter } from 'vue-router'
import { SAVE_GENERATED_SEED } from '~/config/events'
import useWallet from '~/popup/modules/useWallet'
import { isASCII, isLength, isContains } from '~/helpers/index'

const { back, push } = useRouter()
const { getFundAddress, getBalance } = useWallet()
const textarea = ref('')
const password = ref('')
const confirmPassword = ref('')
const usePassphrase = ref(false)
const passphrase = ref('')
const picked = ref('line')
const list = ref([])
const length = ref(12)
const showInfo = ref(false)


const isDisabled = computed(() => {
  const seedArr = textarea.value?.trim()?.split(' ')
  if (seedArr.length !== 12 &&
      seedArr.length !== 15 &&
      seedArr.length !== 18 &&
      seedArr.length !== 21 &&
      seedArr.length !== 24) return true
  if (!textarea.value) return true
  if (!password.value.length || !confirmPassword.value.length) return true
  if (password.value !== confirmPassword.value) return true
  if (!isASCII(password.value) || !isASCII(confirmPassword.value)) return true
  if (!isLength(password.value) || !isLength(confirmPassword.value)) return true
  if (!isContains(password.value) || !isContains(confirmPassword.value)) return true
  return false
})

async function inputWords(e){
  const l = await list.value
  console.log(e,e.target,'e.target.value:',e.target.value,'| l:', l,'|textarea:',textarea.value)

  if(picked.value === 'line'){
    list.value = textarea.value?.trim()?.split(' ')
    if (list.value.length === 12 ||
        list.value.length === 15 ||
        list.value.length === 18 ||
        list.value.length === 21 ||
        list.value.length === 24){
          length.value = list.value.length
          return;
        }
        length.value = 12
    return;
  }
  if(picked.value === 'list'){

    if(e.target.value?.trim()?.split(' ')?.length > list.value.length){
      textarea.value = e.target.value
    }
    if(list.value.length >= e.target.value?.trim()?.split(' ')?.length){
      textarea.value = list.value?.join(' ')?.trim()
    }

    if (list.value.length === 12 ||
        list.value.length === 15 ||
        list.value.length === 18 ||
        list.value.length === 21 ||
        list.value.length === 24){
        length.value = list.value.length
          return;
    }
    return;
  }
  if(!length.value) length.value = 12
  return;
}
function viewShowInfo(){
  showInfo.value = !showInfo.value
  return true
}
/*
function clearTextarea(){
  textarea.value = ''
  list.value = []
  return true
}
*/
async function onStore() {
  const generated = await sendMessage(
    SAVE_GENERATED_SEED,
    {
      seed: textarea.value.replace(/\s\s+/g, ' ').trim(),
      password: password.value,
      passphrase: passphrase.value
    },
    'background'
  )
  if (generated) {
    await getFundAddress()
    getBalance()
    push('/')
  }
}
</script>

<style lang="scss" scoped>
.load-screen {
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
