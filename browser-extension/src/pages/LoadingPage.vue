<template>
  <div class="loading-screen flex flex-col h-full" data-testid="loading-page">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div
      class="loading-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <LoaderIcon class="loader animate-spin" />
      <span class="text-[var(--text-grey-color)]">Loading...</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import LoaderIcon from '~/components/Icons/LoaderIcon.vue'
</script>

<style scoped>
.loading-screen_content {
  padding-top: 22px;
  padding-bottom: 34px;
}

.loading-screen_content .loader {
  width: 56px;
  height: 56px;
  margin-top: 170px;
  margin-bottom: 190px;
}

.loading-screen_content span {
  font-weight: 500;
  font-size: 15px;
  line-height: 20px;
  display: flex;
  align-items: center;
  text-align: center;
  letter-spacing: -0.32px;
}
</style>
