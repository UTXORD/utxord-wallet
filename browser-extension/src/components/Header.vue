<template>
  <div class="header bg-[var(--primary)] w-full flex items-center px-5 py-2">
    <span
      @click="copyToClipboard(fundAddress)"
      class="cursor-pointer text-black"
      >Fund: {{ formatAddress(fundAddress, 6, 6) }}</span
    >
    <div class="header-buttons flex ml-auto">
      <ThemeBtn class="mr-5" />
      <router-link to="/manage">
        <img
          class="header-settings mr-5"
          src="/assets/settings.svg"
          alt="Settings"
          title="Manage account"
        />
      </router-link>
      <Modal @on-submit="unload" title="Logout" submit-text="Yes">
        <template #button="{ onClick }">
          <img
            class="header-logout"
            src="/assets/logout.svg"
            alt="Logout"
            title="Change account"
            @click="onClick"
          />
        </template>
        <template #body>
          <p
            class="text-base text-left leading-relaxed text-[var(--text-grey-color)]"
          >
            Do you want to unload your keys?
          </p>
        </template>
      </Modal>
    </div>
  </div>
</template>

<script setup lang="ts">
import { toRefs } from 'vue'
import { sendMessage } from 'webext-bridge'
import { useRouter } from 'vue-router'
import {UNLOAD_SEED, SET_UP_PASSWORD } from '~/config/events'
import Modal from '~/components/Modal.vue'
import ThemeBtn from '~/components/ThemeBtn.vue'
import { useStore } from '~/popup/store'
import { formatAddress, copyToClipboard } from '~/helpers/index'

const { push } = useRouter()
const store = useStore()
const { fundAddress } = toRefs(store)

function unload() {
  localStorage.removeItem('temp-mnemonic')
  localStorage.removeItem(SET_UP_PASSWORD)
  localStorage.removeItem('SET_UP_PASSWORD_PAGE')
  localStorage.removeItem('SET_UP_PASSWORD_CONFIRM_PAGE')
  store.clearStore()
  sendMessage(UNLOAD_SEED, {}, 'background')
}
</script>

<style scoped>
.header span {
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: -0.01em;
}

.header-buttons img {
  width: 18px;
  height: 18px;
  cursor: pointer;
}
</style>
