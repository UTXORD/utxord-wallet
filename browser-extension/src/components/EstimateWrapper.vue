<template>

  <LoadingPage v-if="loading" />

  <div v-else class="estimate-screen h-full flex flex-col" v-bind="$attrs">
    <Header />
    <Logo />
    <div class="w-full h-[1px] bg-[var(--border-color)]" />
    <div
      class="estimate-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <slot />


      <!-- Fees -->
      <div class="full-width flex column q-gap-sm q-mb-sm">
        <div
          v-for="(t, i) of FEES_TABS"
          :key="i"
          class="estimate-screen_fees-tab flex column"
          :class="{
            'estimate-screen_fees-tab--active': t.type === selectedType,
            'cursor-not-allowed': loading,
          }"
          @click="onSelect(t.type, t.value)"
          v-ripple="!loading"
        >
          <!-- Buttons -->
          <div class="flex items-center justify-between">
            <div class="flex column">
              <span class="estimate-screen_fees-tab-label q-mr-auto">
                {{ t.label }}
              </span>
              <span class="estimate-screen_fees-tab-value">{{ t.time }}</span>
            </div>
            <div class="flex column items-end">
              <span class="estimate-screen_fees-tab-label"
                >{{ t.value?.toFixed(2) || 0 }} sats/vB</span
              >
              <span class="estimate-screen_fees-tab-value">(~$0.20)</span>
            </div>
          </div>

          <!-- Slider -->
          <div
            v-if="t.type === TYPE_CUSTOM"
            class="estimate-screen_fees-tab-custom flex column"
          >
            <q-slider
              v-model="selectedStep"
              color="primary"
              markers
              snap
              :min="0"
              :max="STEPS_COUNT"
              selection-color="transparent"
              :disable="loading"
              @update:model-value="debounceOnSelect(t.type)"
            />
            <div
              class="estimate-screen_fees-tab-custom_labels flex items-center justify-between"
            >
              <span>
                min
                <q-tooltip class="text-caption">{{ minVB }} sats/vB</q-tooltip>
              </span>
              <span>
                max
                <q-tooltip class="text-caption">{{ maxVB }} sats/vB</q-tooltip>
              </span>
            </div>
          </div>
        </div>
      </div>

      <!-- Transaction fee -->
      <div
        class="estimate-screen_transaction_fee flex items-center full-width q-gap-sm q-mb-sm"
      >
        <div class="estimate-screen_transaction_fee-label q-mr-auto">
          {{ transaction_fee.label }}
        </div>
        <div class="estimate-screen_transaction_fee-sats q-ml-sm">{{ transaction_fee.sats }}</div>
        <div class="estimate-screen_transaction_fee-usd">{{ transaction_fee.usd }}</div>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mb-5 mt-auto">
        <Button
          second
          class="min-w-[40px] mr-3 px-0 flex items-center justify-center"
          @click="back"
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
import WinHelpers from '~/helpers/winHelpers'
import { useStore } from '~/popup/store/index'
import LoadingPage from '~/pages/LoadingPage.vue'
import {useRouter} from "vue-router";

const store = useStore()
const { balance, dataForSign } = toRefs(store)
const winHelpers = new WinHelpers()

const { back, push } = useRouter()


const STEPS_COUNT = 10;
const TYPE_NORMAL = 'normal';
const TYPE_PRIORITY = 'priority';
const TYPE_CUSTOM = 'custom';

const loading = ref(false);
const selectedType = ref<string>('');
const selectedStep = ref<number>(0);

const NORMAL_FEE_VAL = 12000;
const PRIORITY_FEE_VAL = 22000;

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
  sats: formatPrice(1172, 'sats'),  // TODO: why 1172 ?
  usd: `(~$${convertSatsToUSD(7700, 10 /* usd.value */)})`,  // TODO: why 7700 ? Why usd.value ?
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
        1000,
        1900,
        STEPS_COUNT,
        vbToBytes(val || 0)
      );
  }
}

const debounce = (func: (e: any) => void, timeout = 300) => {
  let timer: ReturnType<typeof setTimeout>;
  return (...args: [e: any]) => {
    clearTimeout(timer);
    timer = setTimeout(() => {
      func.apply(this, args);
    }, timeout);
  };
};

const debounceOnSelect = debounce(onSelect);

function vbToBytes(kb: number): number {
  return kb * 1000;
}

function bytesToVb(bytes: number): number {
  return bytes / 1000;
}

async function onConfirm() {
    // dataForSign.value = {...dataForSign.value, ...{selectedMiningFee: 0}};
    await push(`//sign-commit-buy`)
}

onMounted(() => {
  setTimeout(() => {
    loading.value = false
  }, 1000)
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
  color: var(--grey-text);
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
    font-size: 16px;
    font-weight: 600;
    line-height: 24px;
    text-align: center;
  }

  &-descr {
    font-size: 14px;
    font-weight: 600;
    line-height: 20px;
    text-align: center;
    color: var(--grey-text);
  }

  &-tab {
    border-radius: 10px;
    border: 1px solid var(--border-bg);
    position: relative;
    cursor: pointer;
    padding: 8px 12px;

    &-label {
      color: #fff;
      font-size: 16px;
      font-style: normal;
      font-weight: 600;
      line-height: 20px; /* 120% */
      letter-spacing: -0.32px;
    }

    &-value {
      color: #aaabad;
      font-size: 14px;
      font-style: normal;
      font-weight: 600;
      line-height: 20px;
      letter-spacing: -0.32px;
    }

    &--active {
      border: 1px solid var(--q-primary);
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
      font-weight: 600;
      line-height: 20px;
      text-align: left;
      color: var(--grey-text);
    }

    &-sats {
      @include satsStyle;
    }

    &-usd {
      @include usdStyle;
    }
  }

  &_btns {
    :deep(.q-btn) {
      height: 40px;
      font-size: 16px;
      border-radius: 8px;
    }

    &-collect {
      color: #000;
      background-color: var(--q-primary);

      &.disabled {
        color: rgba($color: #000, $alpha: 0.24);
        background-color: rgba($color: #000, $alpha: 0.04) !important;
        box-shadow: none;
        border: 1px solid rgba($color: #000, $alpha: 0.1);

        .q-btn__content {
          color: rgba($color: #000, $alpha: 0.4);
        }
      }
    }

    &-start {
      color: #fff;
      background-color: #2b2d33;
      box-shadow: 0px 20px 80px 12px var(--q-primary);
    }
  }

  :deep(.q-slider__track) {
    height: 6px !important;
    background: transparent;
    border-radius: 0 !important;
  }

  :deep(.q-slider__inner) {
    margin-top: 2px;
    background: #404247;
    height: 2px;
    width: 100%;
  }

  :deep(.q-slider__markers) {
    color: #404247;
    width: calc(100% - 6px) !important;
    overflow: visible !important;

    &:after {
      width: 6px;
      right: -6px;
    }
  }

  :deep(.q-slider__thumb) {
    &::after {
      content: '';
      width: 6px;
      height: 6px;
      z-index: 10;
      position: absolute;
      left: 50%;
      top: 50%;
      display: block;
      background: #000;
      border-radius: 100%;
      transform: translate(-50%, -50%);
    }
  }

  :deep(.q-slider__markers--h) {
    background-image: repeating-linear-gradient(
      to right,
      currentColor,
      currentColor 6px,
      rgba(255, 255, 255, 0) 0,
      rgba(255, 255, 255, 0)
    );
  }
}

.body--dark {
  .estimate-screen_fees {
    &_btns {
      &-collect {
        &.disabled {
          color: rgba($color: #fff, $alpha: 0.24);
          background-color: rgba($color: #fff, $alpha: 0.04) !important;
          box-shadow: none;
          border: 1px solid rgba($color: #fff, $alpha: 0.1);

          .q-btn__content {
            color: rgba($color: #fff, $alpha: 0.4);
          }
        }
      }

      &-start {
        color: #000;
        background-color: #fff;
      }
    }
  }
}
</style>
