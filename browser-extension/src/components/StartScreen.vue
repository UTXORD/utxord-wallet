<template>
  <div class="start-screen flex flex-col h-full">
    <Logo />
    <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
    <div class="start-screen_content h-full flex flex-col justify-between px-5 pb-5">
      <div class="start-screen_lock-key flex flex-col items-center">
        <img
          class="start-screen_lock-key_img mb-5"
          src="/assets/lockKey.svg"
          alt="Lock"
        />
        <span>Welcome to the UTXORD Wallet Extention</span>
      </div>
      <div class="flex flex-col items-center">
        <Button
          to="/create-password-screen#alert-mnemonic"
          class="w-full mb-3"
        >
          Create a new wallet
        </Button>
        <Button
          to="/create-password-screen#load"
          class="w-full"
          second
        >
          I already have a wallet
        </Button>
      </div>
    </div>
  </div>
</template>

<style lang="scss" scoped>
.start-screen {
  &_lock-key {
    padding-top: 90px;
    padding-bottom: 90px;

    &_img {
      height: 88px;
      width: auto;
    }
  }

  &_lock-key span {
    max-width: 280px;
    font-weight: 600;
    font-size: 22px;
    line-height: 30px;
    letter-spacing: 0.33px;
  }

  &_form span {
    font-weight: 400;
    font-size: 15px;
    line-height: 20px;
    letter-spacing: -0.01em;
  }
}
</style>
