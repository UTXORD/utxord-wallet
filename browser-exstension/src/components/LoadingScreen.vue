<template>
  <div class="loading-screen flex flex-col h-full">
    <Logo />
    <div
      class="loading-screen_content h-full flex flex-col items-center px-5 pb-5"
    >
      <img src="/assets/loader.svg" class="animate-spin" alt="Loading..." />
      <span>Loading...</span>
    </div>
  </div>
</template>

<style scoped>
.loading-screen_content {
  padding-top: 22px;
  padding-bottom: 34px;
  border-top: 1px solid #e8e8e8;
}

.loading-screen_content img {
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
  color: #737375;
}
</style>
