<template>
  <div class="load-screen flex flex-col h-full">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="load-screen_content h-full flex flex-col items-center px-5 pb-5">
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
        <div class="flex items-center mb-2 hidden">
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
        <CustomInput
          v-if="picked == 'line'"
          @change="inputWords"
          type="textarea"
          class="w-full"
          autofocus
          rows="3"
          placeholder="Enter your phrase here"
          v-model="textarea"
        />
        <table style="width: 100%;" v-if="picked == 'list'">
        <!-- for 12 words -->
          <tbody v-if="length == 12" v-for="n in 4">
          <tr>
            <td>{{n}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n-1]"/></td>
            <td>{{n+4}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+3]"/></td>
            <td>{{n+8}}.&nbsp;<input class="bg-[var(--bg-color)] text-[var(--text-color)]" size="10" type="text" @change="inputWords" v-model="list[n+7]"/></td>
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
      </div>
      <!-- Passphrase -->
      <div class="w-full h-[1px] bg-[#404247] my-5" />

      <!-- I saved my mnemonic -->
      <div class="w-full flex justify-start items-center gap-2 mb-5">
        <Checkbox v-model="usePassphrase">
          <span class="w-full text-[var(--text-grey-color)] text-[15px]">
            Use Passphrase
          </span>
        </Checkbox>
        <VTooltip :distance="10">
          <QuestionIcon class="cursor-pointer" />
          <template #popper>
            <div class="max-w-[250px] load-screen_tooltip-descr">
              A passphrase is not a password; it behaves differently. It will be accepted if you already have a passphrase and enter it wrong. However, it will produce an empty new set of addresses.
            </div>
          </template>
        </VTooltip>
      </div>

      <!-- Password field -->
      <div
        class="load-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
        v-show="usePassphrase"
      >
        <div class="load-screen_form-input flex flex-col">
          <span class="mb-2 w-full text-[var(--text-grey-color)]">Passphrase (optional)</span>
          <CustomInput
            type="text"
            v-model="passphrase"
            name="passphrase"
          />
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="back"
        >
          <ArrowLeftIcon />
        </Button>
        <Button @click="onStore" :disabled="isDisabled" class="w-full">Confirm</Button>
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
import { SAVE_GENERATED_SEED } from '~/config/events'
import useWallet from '~/popup/modules/useWallet'
import {
  vTooltip
} from 'floating-vue'

const { back, push } = useRouter()
const { getFundAddress, getBalance } = useWallet()
const textarea = ref('')
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
  console.log('passphrase.value-toSave:', passphrase.value, usePassphrase.value)
  const generated = await sendMessage(
    SAVE_GENERATED_SEED,
    {
      seed: textarea.value.replace(/\s\s+/g, ' ').trim(),
      passphrase: passphrase.value
    },
    'background'
  )
  if (generated) {
    await getFundAddress()
    getBalance()
    push('/loading#wallet-created')
  }
}
</script>

<style lang="scss" scoped>
.load-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;
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
}
</style>
