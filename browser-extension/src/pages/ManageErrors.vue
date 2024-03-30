<template>
  <div class="manage-errors-screen flex flex-col h-full">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="manage-errors-screen_content h-full flex flex-col items-start px-5">

      <h1 class="text-[var(--text-color)] text-[18px] mb-4">Errors Setup</h1>

      <!-- Radio buttons -->
      <div class="flex flex-col gap-3 mb-7">
        <RadioBox v-model="errorReporting" :value="false" @update:model-value="onChangeErrorReporting">
          <span
            class="text-[var(--text-grey-color)] text-[15px]"
            :class="{
              '!text-[var(--text-color)]': errorReporting === false
            }"
          >
            Disable error report submission
          </span>
        </RadioBox>
        <RadioBox v-model="errorReporting" :value="true" @update:model-value="onChangeErrorReporting">
          <span
            class="text-[var(--text-grey-color)] text-[15px]"
            :class="{
              '!text-[var(--text-color)]': errorReporting === true
            }"
          >
            Enable error report submission
          </span>
        </RadioBox>
      </div>

      <!-- Buttons -->
      <div class="flex w-full mt-auto">
        <Button
          second
          class="w-full px-0 flex items-center justify-center gap-2"
          @click="back"
        >
          Go Back
        </Button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { sendMessage } from 'webext-bridge'
import { toRefs} from 'vue'
import { useRouter } from 'vue-router'
import { useStore } from '~/popup/store/index'
import { CHANGE_ERROR_REPORTING, STATUS_ERROR_REPORTING  } from '~/config/events';
import { onMounted } from "vue";

const store = useStore()
const { errorReporting } = toRefs(store)
const { back, push } = useRouter()

async function initErrorReporting() {
    const res = await sendMessage(STATUS_ERROR_REPORTING, {}, 'background')
    store.setErrorReporting(Boolean(res.error_reporting));
}

async function onChangeErrorReporting() {
  const res = await sendMessage(
    'CHANGE_ERROR_REPORTING',
    {
      value: Boolean(errorReporting.value)
    },
    'background')
    store.setErrorReporting(Boolean(errorReporting.value))
}

onMounted(async () => {
  await initErrorReporting();
})
</script>

<style lang="scss" scoped>
.manage-errors-screen {
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
