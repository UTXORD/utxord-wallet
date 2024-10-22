<template>

  <LoadingPage v-if="loading" />

  <div v-else class="estimate-screen h-full flex flex-col" v-bind="$attrs">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="estimate-screen_content h-full flex flex-col items-center px-5 pb-5">
      <slot />

      <!-- Title and description -->
      <div class="w-full flex flex-col gap-2 mb-4">
        <div class="estimate-screen_fees-title">
          Choose your transaction speed and fee
        </div>
        <div class="estimate-screen_fees-descr">
          The USD values are approximate.
        </div>
      </div>

      <!-- Fees -->
      <div class="w-full flex flex-col gap-2 mb-4">
        <div
          v-for="(t, i) of FEES_TABS"
          :key="i"
          class="estimate-screen_fees-tab flex flex-col"
          :class="{
            'estimate-screen_fees-tab--active': t.type === selectedType,
            'cursor-not-allowed': loading,
          }"
          @click="onSelect(t.type, t.value)"
          v-ripple="!loading"
        >
          <!-- Buttons -->
          <div class="flex items-center justify-between">
            <div class="flex flex-col items-start">
              <span class="estimate-screen_fees-tab-label mr-auto">
                {{ t.label }}
              </span>
              <span class="estimate-screen_fees-tab-value">{{ t.time }}</span>
            </div>
            <div class="flex flex-col items-end">
              <span class="estimate-screen_fees-tab-label"
                >{{ t.value || 0 }} sats/vB</span
              >
              <span class="estimate-screen_fees-tab-value">
                (~${{ convertSatsToUSD(vbToBytes(t.value), usdRate) }})
              </span>
            </div>
          </div>

          <!-- Slider -->
          <div
            v-if="t.type === TYPE_CUSTOM"
            class="estimate-screen_fees-tab-custom flex flex-col"
          >
          <div class="estimate-screen_fees-slider mt-1">
            <RangeSlider
              v-model="selectedStep"
              :min="0"
              :max="STEPS_COUNT"
              step="1"
              sticky
            />
            <div class="estimate-screen_fees-slider-points w-full flex items-center justify-between">
              <div
                v-for="i of STEPS_COUNT + 1"
                :key="i"
                class="estimate-screen_fees-slider-point"
              />
            </div>
          </div>
            <div
              class="estimate-screen_fees-tab-custom_labels flex items-center justify-between"
            >
              <VDropdown :triggers="['hover']" :distance="10" placement="bottom">
                <span class="cursor-pointer">min</span>
                <template #popper>
                  <div class="max-w-[250px] load-screen_tooltip-descr bg-black py-2 px-3 text-white">
                    {{ minVB }} sats/vB
                  </div>
                </template>
              </VDropdown>
              <VDropdown :triggers="['hover']" :distance="10" placement="bottom">
                <span class="cursor-pointer">max</span>
                <template #popper>
                  <div class="max-w-[250px] load-screen_tooltip-descr bg-black py-2 px-3 text-white">
                    {{ maxVB }} sats/vB
                  </div>
                </template>
              </VDropdown>
            </div>
          </div>
        </div>
      </div>

      <!-- Transaction fee -->
      <div
        class="estimate-screen_fees_total flex items-center w-full gap-2 mb-8"
      >
        <div class="estimate-screen_fees_total-label mr-auto">
          {{ transaction_fee.label }}
        </div>
        <div class="estimate-screen_fees_total-sats ml-2">{{ transaction_fee.sats }}</div>
        <div class="estimate-screen_fees_total-usd">{{ transaction_fee.usd }}</div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mb-5 mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center"
          @click="goToBack"
          data-testid="go-back"
        >
          <ArrowLeftIcon />
        </Button>
        <Button @click="onConfirm" enter class="w-full" data-testid="confirm">
          Confirm
        </Button>
      </div>
    </div>
  </div>

</template>

<script setup lang="ts">

import { ref, toRefs, computed, onMounted } from 'vue'
import { useStore } from '~/popup/store/index'
import LoadingPage from '~/pages/LoadingPage.vue'
import {useRouter} from "vue-router";
import RangeSlider from "vue3-slider";
import useWallet from '~/popup/modules/useWallet';

const store = useStore()
const { dataForSign } = toRefs(store)
const { usdRate } = useWallet();

const { back, push } = useRouter()


const STEPS_COUNT = 10;
const TYPE_NORMAL = 'normal';
const TYPE_PRIORITY = 'priority';
const TYPE_CUSTOM = 'custom';

const loading = ref(false);
const selectedType = ref<string>('');
const selectedStep = ref<number>(0);

const NORMAL_FEE_VAL = 12152;
const PRIORITY_FEE_VAL = 22396;

const selectedFeeRate = computed(
  () => NORMAL_FEE_VAL + ((PRIORITY_FEE_VAL - NORMAL_FEE_VAL) / STEPS_COUNT) * selectedStep.value
);
const hoursStart = computed(() => NORMAL_FEE_VAL + ((PRIORITY_FEE_VAL - NORMAL_FEE_VAL) / STEPS_COUNT) * 2);
const minutesStart = computed(
  () => NORMAL_FEE_VAL + ((PRIORITY_FEE_VAL - NORMAL_FEE_VAL) / STEPS_COUNT) * 8
);
const normalText = computed(() => handleText(NORMAL_FEE_VAL));
const priorityText = computed(() => handleText(PRIORITY_FEE_VAL));
const customText = computed(() => {
  if (selectedStep.value < 3) return 'days';
  if (selectedStep.value > 7) return 'minutes';
  return 'hours';
});
const FEES_TABS = computed(() => [
  {
    label: 'Normal',
    value: bytesToVb(NORMAL_FEE_VAL),
    time: normalText.value,
    type: TYPE_NORMAL,
  },
  {
    label: 'Priority',
    value: bytesToVb(PRIORITY_FEE_VAL),
    time: priorityText.value,
    type: TYPE_PRIORITY,
  },
  {
    label: 'Custom',
    value: bytesToVb(selectedFeeRate.value),
    time: customText.value,
    type: TYPE_CUSTOM,
  },
]);

const ONE_BTC_IN_SATOSHI = 100000000;

const USDollar = new Intl.NumberFormat(/* format || */ 'en-US', {
  style: 'decimal',
  currency: 'USD',
});

function formatPrice(price: number, symbol?: string) {
  const sum = price && typeof price === 'number' ? price : 0;
  return `${USDollar.format(sum)} ${symbol || 'SAT'}`;
}

function convertSatsToUSD(
  sats: number,
  usd: number,
  fixed?: number | undefined
): number | string {
  if (!sats || !usd) return 0;
  const sum = (sats || 0) * (1 / ONE_BTC_IN_SATOSHI) * (usd || 0);
  if (!sum) return 0;
  return formatPrice(+sum.toFixed(fixed || 2), ' ');
}

const minVB = computed(() => bytesToVb(NORMAL_FEE_VAL).toFixed(2));
const maxVB = computed(() => bytesToVb(PRIORITY_FEE_VAL).toFixed(2));
const transaction_fee = computed(() => ({
  label: 'Transaction fee',
  sats: `${bytesToVb(selectedFeeRate.value)} sats`,
  usd: `(~$${convertSatsToUSD(selectedFeeRate.value, usdRate.value)})`,
}));

function handleText(value: number) {
  if (hoursStart.value >= value) return 'days';
  if (minutesStart.value <= value) return 'minutes';
  return 'hours';
}

function calculateStep(
  minValue: number,
  maxValue: number,
  numSteps: number,
  value: number
) {
  const step = (maxValue - minValue) / numSteps;
  const stepNumber = Math.round((value - minValue) / step);
  return stepNumber;
}

function onSelect(type: string, val?: number) {
  if (!loading.value) {
    selectedType.value = type;
    if (val)
      selectedStep.value = calculateStep(
        NORMAL_FEE_VAL,
        PRIORITY_FEE_VAL,
        STEPS_COUNT,
        vbToBytes(val || 0)
      );
  }
}

function vbToBytes(kb: number): number {
  return kb * 1000;
}

function bytesToVb(bytes: number): number {
  return +((bytes / 1000).toFixed(2).replace(/[.,]0+$/, ''));
}

async function onConfirm() {
       dataForSign.value = {
       ...dataForSign.value,
       ...{
       fee_rate: selectedFeeRate.value,
        },
       };

console.log(
  'selectedStep:',selectedStep.value,
  ' selectedType:',selectedType.value,
  ' selectedFeeRate:',selectedFeeRate.value
)

    console.log('onConfirm()->dataForSign:', dataForSign.value);
    if(!dataForSign.value?.location){
      await push(`/sign-commit-buy`)
    }else{
      await push(dataForSign.value?.location)
    }
}

async function goToBack(){
  if(!dataForSign.value?.back){
    back()
  }else{
    await push(dataForSign.value?.back)
  }
}

onMounted(() => {
console.log('dataForSign:', dataForSign.value);
  setTimeout(() => {
    loading.value = false
  }, 500)
})
</script>

<style lang="scss" scoped>

.estimate-screen_content {
  padding-top: 22px;
  padding-bottom: 22px;
}

@mixin usdStyle() {
  font-size: 12px;
  font-weight: 500;
  line-height: 16px;
  letter-spacing: -0.08px;
  text-align: right;
  color: var(--text-grey-color);
}

@mixin satsStyle() {
  font-size: 15px;
  font-weight: 600;
  line-height: 20px;
  letter-spacing: -0.24px;
  text-align: right;
}

.estimate-screen_fees {
  flex: 1;

  &-title {
    font-size: 18px;
    line-height: 24.59px;
    text-align: left;
  }

  &-descr {
    font-size: 15px;
    font-weight: 400;
    line-height: 20px;
    letter-spacing: -0.32px;
    text-align: left;
    color: var(--text-grey-color);
  }

  &-tab {
    border-radius: 10px;
    border: 1px solid var(--border-color);
    position: relative;
    cursor: pointer;
    padding: 8px 12px;

    &-label {
      color: #fff;
      font-size: 16px;
      line-height: 24px;
      text-align: left;
    }

    &-value {
      color: #aaabad;
      font-size: 14px;
      line-height: 20px;
      text-align: left;
    }

    &--active {
      border: 1px solid var(--primary);
    }

    &-custom_labels {
      span {
        font-size: 12px;
        font-style: normal;
        font-weight: 500;
        line-height: 16px;
        letter-spacing: -0.078px;
        color: #aaabad;
      }
    }
  }

  &_total {
    &-label {
      font-size: 14px;
      line-height: 20px;
      text-align: left;
      color: var(--text-grey-color);
    }

    &-sats {
      @include satsStyle;
    }

    &-usd {
      @include usdStyle;
    }
  }

  &-slider {
    position: relative;

    .vue3-slider {
      height: 18px;
      margin: 0;
      z-index: 2;

      :deep(.handle) {
        width: 13px;
        height: 13px;
        background: #000;
        border-radius: 100%;
        border: 3px solid var(--primary);
        top: 50%;
        transform: translateY(-50%) scale(1);
      }

      :deep(.track) {
        height: 2px;
        background: var(--border-color);
        top: 50%;
        transform: translateY(-50%);
        position: absolute;
      }

      :deep(.track-filled) {
        height: 2px;
        background: transparent;
      }
    }

    &-points {
      position: absolute;
      top: 50%;
      transform: translateY(-50%);
      width: 100%;
      pointer-events: none;
      user-select: none;
      z-index: 1;
    }

    &-point {
      width: 5px;
      height: 5px;
      border-radius: 100%;
      background: var(--border-color);
    }
  }
}
</style>
