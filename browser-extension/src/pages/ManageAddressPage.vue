<template>
  <div class="manage-address-screen flex flex-col h-full" data-testid="manage-address-page">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="manage-address-screen_content h-full flex flex-col items-start px-5">

      <h1 class="text-[var(--text-color)] text-[18px] mb-4">Address Setup</h1>

      <!-- Radio buttons -->
      <div class="flex flex-col gap-3 mb-7">
        <RadioBox v-model="typeAddress" :value="TAPROOT_VALUE" @update:model-value="onChangeTypeAddress" data-testid="type-taproot">
          <span
            class="text-[var(--text-grey-color)] text-[15px]"
            :class="{
              '!text-[var(--text-color)]': typeAddress === TAPROOT_VALUE
            }"
          >
            Taproot
          </span>
        </RadioBox>
        <RadioBox v-model="typeAddress" :value="SEGWIT_VALUE" @update:model-value="onChangeTypeAddress" data-testid="type-segwit">
          <span
            class="text-[var(--text-grey-color)] text-[15px]"
            :class="{
              '!text-[var(--text-color)]': typeAddress === SEGWIT_VALUE
            }"
          >
            Segwit
          </span>
        </RadioBox>
      </div>

      <div @click="openedAdvanced = !openedAdvanced" class="flex items-center gap-2 mb-3 cursor-pointer w-full" data-testid="advanced-options">
        <ChevronIcon class="w-[20px]" :class="{ 'transform rotate-180': openedAdvanced }" />
        <span class="text-[15px] text-[var(--text-color)]">Advanced Options</span>
      </div>

      <div v-show="openedAdvanced" class="w-full flex flex-col bg-[var(--section)] rounded-xl px-3 py-3 mb-5">
        <span class="mb-2 w-full text-[var(--text-grey-color)] text-left text-[14px]">Manage derivation</span>
        <div class="flex flex-col bg-[var(--bg-color)] rounded-xl px-4">
          <div
            v-for="(adv, i) in ADVANCED_LIST"
            :key="i"
            class="flex justify-between items-center py-4"
            :class="{ 'border-b-[1px] border-[var(--border-color)]': ADVANCED_LIST.length - 1 !== i }"
          >
            <RadioBox
                v-model="useDerivation"
                :value="adv.value"
                class="w-full"
                @update:model-value="onChangeUseDerivation"
                :data-testid="adv.testId"
            >
              <template #left>
                <div class="flex flex-col w-full">
                  <div class="flex mb-1 gap-1">
                    <span class="text-[15px] text-[var(--text-color)]">{{ adv.label }}</span>
                    <VDropdown :triggers="['hover']" :distance="10" placement="top">
                      <QuestionIcon class="question-icon cursor-pointer" />
                      <template #popper>
                        <div class="max-w-[250px] load-screen_tooltip-descr bg-black py-2 px-3 text-white">
                          {{ adv.tooltip }}
                        </div>
                      </template>
                    </VDropdown>
                  </div>
                  <span class="text-[13px] text-[var(--text-grey-color)] text-left">{{ adv.description }}</span>
                </div>
              </template>
            </RadioBox>
          </div>
        </div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mt-auto">
        <Button
          second
          class="w-full px-0 flex items-center justify-center gap-2"
          @click="push('/manage')"
          data-testid="go-back"
        >
          Go Back
        </Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, toRefs} from 'vue'
import { useRouter } from 'vue-router'
import { isASCII } from '~/helpers/index'
import { sendMessage } from '~/helpers/messenger'
import { useStore } from '~/popup/store/index'
import {BALANCE_CHANGE_PRESUMED, CHANGE_TYPE_FUND_ADDRESS, STATUS_DERIVATION} from '~/config/events';
import useWallet from '~/popup/modules/useWallet'

const TAPROOT_VALUE = 0
const SEGWIT_VALUE = 1

const ADVANCED_LIST = [
  {
    label: 'No Derivation',
    value: false,
    description: 'Generate one static address',
    tooltip: 'More compatible with other ordinal wallets. Pseudo–anonymously discloses what you own, better stats for community. Bitcoin phased it out (2012-19) but will support forever.',
    testId: 'no-derivation'
  },
  {
    label: 'Use Derivation',
    value: true,
    description: 'Autogenerate one-time addresses',
    tooltip: 'Bitcoin recommended. Some ordinal wallets do not support it. No one will tell if any two of your inscriptions belong to the same owner.  See BIP-32.',
    testId: 'use-derivation'
  }
]

const store = useStore()
const { fundAddress, useDerivation, typeAddress } = toRefs(store)
const { getBalance, fetchUSDRate } = useWallet()
const { back, push } = useRouter()

const openedAdvanced = ref(false)

function refreshBalance() {
  store.setSyncToFalse()
  fetchUSDRate()
  setTimeout(async () => {
    const res = await sendMessage(STATUS_DERIVATION, {}, 'background')
    await store.setUseDerivation(Boolean(res.derivate))
    const ta = Number(typeAddress.value);
    const tl = Boolean(useDerivation.value)?'fund':'oth'
    const addr = await res.keys?.addresses?.reverse()?.find(
      (item) => item.type === tl && item.typeAddress === ta
    )?.address
    await store.setFundAddress(addr)
    await getBalance(fundAddress.value);
  }, 1000)
}

async function onChangeTypeAddress(){
  await sendMessage(BALANCE_CHANGE_PRESUMED, {}, 'background')
  const ta = Number(typeAddress.value);
  store.setTypeAddress(ta);
  const response = await sendMessage(
    CHANGE_TYPE_FUND_ADDRESS,
    {
      type: ta
      },
      'background'
  )
  const tl = Boolean(useDerivation.value) ? 'fund' : 'oth'
  const addr = await response?.addresses?.reverse()?.find(
    (item) => item.type === tl && item.typeAddress === ta
    )?.address
  await store.setFundAddress(addr)
  await refreshBalance();
}

async function onChangeUseDerivation(){
  const res = await sendMessage(
    'CHANGE_USE_DERIVATION',
    {
      value: Boolean(useDerivation.value)
    },
    'background')
    store.setUseDerivation(Boolean(useDerivation.value))
    const ta = Number(typeAddress.value);
    const tl = Boolean(useDerivation.value)?'fund':'oth'
    const addr = res.keys?.addresses?.reverse()?.find(
      (item) => item.type === tl && item.typeAddress === ta
    )?.address
    store.setFundAddress(addr)
}

</script>

<style lang="scss" scoped>
.manage-address-screen {
  &_content {
    padding-top: 22px;
    padding-bottom: 22px;

    p {
      font-size: 18px;
      line-height: 25px;
      margin-bottom: 15px;
    }
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
    color: #1b46f5;
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
