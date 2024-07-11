<template>
<!-- Total -->
<div
  class="sign-screen_block w-full flex items-center bg-[var(--section)] rounded-lg p-3 mb-5"
  v-show="raws?.length > 0"
  data-testid="get-raw-tx-description"
>
  <span><a @click="whatSigning" class="mr-2 text-[var(--text-color)]">What am I signing?</a></span>
  <span class="w-full text-[var(--text-grey-color)]"
    >Description:</span
  >
  <span class="mr-2 text-[var(--text-color)]" >...</span>
</div>
<!-- What am I signing -->
<div
  class="generate-screen_form w-full flex flex-col bg-[var(--section)] rounded-xl p-3 mb-5"
  v-if="showContract"
  data-testid="get-raw-tx-contract"
>
  <div class="flex items-center mb-2">
    <span class="w-full text-[var(--text-grey-color)]"
      >Raw transcation:</span
    >
    <CopyIcon
      class="cursor-pointer"
      @click="copyToClipboard(textarea, 'Constant was copied!')"
    />
  </div>
  <span>
  <a v-for="(item, index) in raws" @click="showRawTranscation(index)" :active="activeTab === index" style="float:left;" class="mr-2 text-[var(--text-color)]" data-testid="raw-tx-link">#raw_transcation_{{ index+1 }}</a>
  </span>
  <CustomInput
    type="textarea"
    class="w-full"
    rows="3"
    v-model="textarea"
    readonly
    data-testid="raw-tx-dump"
  />
</div>

</template>

<script setup lang="ts">
import { toRefs, computed } from 'vue'
import { copyToClipboard } from '~/helpers/index'
import { useStore } from '~/popup/store/index'
import CopyIcon from '~/components/Icons/CopyIcon.vue'

const store = useStore()
const { dataForSign } = toRefs(store)
const textarea = ref('')
const showContract = ref('')
const activeTab = ref('')

const raws = computed(() => {
  return dataForSign.value?.data?.costs?.raw || dataForSign.value?.data?.raw
})

async function showRawTranscation(n){
  showContract.value = true;
  textarea.value = raws.value[n]
  activeTab.value = Number(n)
}

async function whatSigning(){
    activeTab.value = 0;
  if(showContract.value){
    showContract.value = false;
    textarea.value = '';
    return;
  }
  showContract.value = true;
  textarea.value = raws.value[0]
}
</script>

<style scoped>
[active=true] {
  color: var(--text-grey-color);
}
.sign-screen_block span {
  text-align: left;
  font-weight: 400;
  font-size: 14px;
  line-height: 18px;
  letter-spacing: -0.154px;
}
</style>
