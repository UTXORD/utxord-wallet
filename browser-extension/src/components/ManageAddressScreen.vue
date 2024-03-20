<template>
  <div class="password-screen flex flex-col h-full">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="password-screen_content h-full flex flex-col items-start px-5">

      <!-- Inputs -->

      <p class="text-[var(--text-color)]">Address Setup</p>


      <input type="radio" id="radio-taproot" value="0" :checked="typeAddress === 0" v-model="typeAddress" @change="setTypeAddress">
      <label for="one">TapRoot</label>
      <br>
      <input type="radio" id="radio-segwit"  value="1" :checked="typeAddress === 1" v-model="typeAddress" @change="setTypeAddress">
      <label for="two">SegWit</label>

      <p class="text-[var(--text-color)]">Advanced Options</p>
      <p class="text-[var(--text-color)]">Manage Derivation</p>

      <input type="radio" id="radio-derivation-static" value="" :checked="useDerivation === false" v-model="useDerivation" @change="setDerivate">
      <label for="one">No Derivation<span> Generate one static address</span><span class="tooltip"> More compatible with other ordinal wallets. Pseudo–anonymously discloses what you own, better stats for community. Bitcoin phased it out (2012-19) but will support forever.</span></label>
      <br>
      <input type="radio" id="radio-derivation-onetime"  value="1" :checked="useDerivation === true" v-model="useDerivation" @change="setDerivate">
      <label for="two">Use Derivation<span> Autogenerate one-time addresses</span><span class="tooltip"> Bitcoin recommended. Some ordinal wallets do not support it. No one will tell if any two of your inscriptions belong to the same owner.  See BIP-32.</span></label>


      <!-- Buttons -->
      <div class="flex w-full mt-auto">
        <Button
          outline
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center bg-white"
          @click="back"
        >
          <img src="/assets/arrow-left.svg" alt="Go Back" />
        </Button>
        <p>You have chosen to use {{useDerivation === true ? "one-time" : "static"}} {{typeAddress == 0 ? "TapRoot" : "SegWit"}} address</p>>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { sendMessage } from 'webext-bridge'
import { computed, ref, toRefs} from 'vue'
import { useRouter } from 'vue-router'
import { isASCII } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import {BALANCE_CHANGE_PRESUMED, CHANGE_TYPE_FUND_ADDRESS, STATUS_DERIVATION} from "~/config/events";
import useWallet from "~/popup/modules/useWallet"

const store = useStore()
const {fundAddress, useDerivation, typeAddress } = toRefs(store)
const { getBalance, fetchUSDRate } = useWallet()

const { back, push } = useRouter()

// const radioTypeAddress = ref('')

function refreshBalance() {
  store.setSyncToFalse()
  fetchUSDRate()
  setTimeout(async () => {
    const res = await sendMessage(STATUS_DERIVATION, {}, 'background')
    store.setUseDerivation(Boolean(res.derivate))
    const ta = Number(!typeAddress.value);
    const tl = Boolean(useDerivation.value)?'fund':'oth'
    const addr = res.keys?.addresses?.reverse()?.find(
      (item) => item.type === tl && item.typeAddress === ta
    )?.address
    store.setFundAddress(addr)
    await getBalance(fundAddress.value);
  }, 1000)
}

async function setTypeAddress(typeAddr){
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
  const tl = Boolean(useDerivation.value)?'fund':'oth'
  const addr = response?.addresses?.reverse()?.find(
    (item) => item.type === tl && item.typeAddress === ta
    )?.address
  store.setFundAddress(addr)
  await refreshBalance();
}

async function setDerivate(){
  const res = await sendMessage(
    'CHANGE_USE_DERIVATION',
    {
      value: Boolean(useDerivation.value)
    },
    'background')
    store.setUseDerivation(Boolean(useDerivation.value))
    const ta = Number(!typeAddress.value);
    const tl = Boolean(useDerivation.value)?'fund':'oth'
    const addr = res.keys?.addresses?.reverse()?.find(
      (item) => item.type === tl && item.typeAddress === ta
    )?.address
    store.setFundAddress(addr)
}

</script>

<style lang="scss" scoped>
.password-screen {
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
}
</style>
