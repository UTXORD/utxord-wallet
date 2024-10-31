<template>
  <div class="manage-view-mode-screen flex flex-col h-full" data-testid="manage-view-mode-page">
    <Header />
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="manage-view-mode-screen_content h-full flex flex-col items-start px-5" data-testid="error-report-submission">

      <h1 class="text-[var(--text-color)] text-[18px] mb-4">Panel Setup</h1>

      <!-- Radio buttons -->
      <div class="flex flex-col gap-3 mb-7">
        <RadioBox v-model="viewMode" :value="false" @update:model-value="onChangeViewMode" data-testid="false">
          <span
            class="text-[var(--text-grey-color)] text-[15px]"
            :class="{
              '!text-[var(--text-color)]': viewMode === false
            }"
          >
            Enable popup mode
          </span>
        </RadioBox>
        <RadioBox v-model="viewMode" :value="true" @update:model-value="onChangeViewMode" data-testid="true">
          <span
            class="text-[var(--text-grey-color)] text-[15px]"
            :class="{
              '!text-[var(--text-color)]': viewMode === true
            }"
          >
            Enable Side Panel mode
          </span>
        </RadioBox>
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
import { sendMessage } from '~/helpers/index'
import { toRefs} from 'vue'
import { useRouter } from 'vue-router'
import { useStore } from '~/popup/store/index'
import { CHANGE_VIEW_MODE, STATUS_VIEW_MODE  } from '~/config/events';
import { onMounted } from "vue";

const store = useStore()
const { viewMode } = toRefs(store)
const { back, push } = useRouter()

async function initViewMode() {
    const res = await sendMessage(STATUS_VIEW_MODE, {}, 'background')
    console.log('STATUS_VIEW_MODE:',res.viewMode)
    store.setViewMode(Boolean(res.viewMode));
    await chrome.sidePanel.setOptions({ enabled: Boolean(res.viewMode)})
    if(Boolean(res.viewMode) === true){
        chrome.sidePanel
          .setPanelBehavior({ openPanelOnActionClick: Boolean(res.viewMode) })
          .catch((error) => console.error(error));
    }
}

async function onChangeViewMode() {
console.log('onChangeViewMode:',viewMode)
  const res = await sendMessage(
    'CHANGE_VIEW_MODE',
    {
      value: Boolean(viewMode.value)
    },
    'background')
    store.setViewMode(Boolean(viewMode.value))
    await chrome.sidePanel.setOptions({ enabled: Boolean(viewMode.value)})
    if(Boolean(viewMode.value) === true){
        chrome.sidePanel
          .setPanelBehavior({ openPanelOnActionClick: Boolean(viewMode.value) })
          .catch((error) => console.error(error));
    }
}

onMounted(async () => {
  await initViewMode();
})
</script>

<style lang="scss" scoped>
.manage-view-mode-screen {
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
