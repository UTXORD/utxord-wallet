<template>
  <NotificationGroup group="success">
    <div
      class="fixed top-30 left-0 w-full flex items-start justify-end px-4 z-10"
      data-testid="notify-success"
    >
      <div class="w-full max-w-sm">
        <Notification
          v-slot="{ notifications, close }"
          enter="transform ease-out duration-300 transition"
          enter-from="translate-y-[-40px] opacity-0 sm:translate-y-0 sm:translate-x-4"
          enter-to="translate-y-0 opacity-100 sm:translate-x-0"
          leave="transition ease-in duration-500"
          leave-from="opacity-100"
          leave-to="opacity-0"
          move="transition duration-500"
          move-delay="delay-300"
        >
          <div
            class="relative flex w-full max-w-sm mx-auto mt-4 overflow-hidden bg-[var(--bg-color)] rounded-lg shadow-md"
            v-for="notification in notifications"
            :key="notification.id"
          >
            <div
              class="flex items-center justify-center w-12 bg-[var(--primary)]"
            >
              <svg
                class="w-6 h-6 text-white fill-current"
                viewBox="0 0 40 40"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path
                  d="M20 3.33331C10.8 3.33331 3.33337 10.8 3.33337 20C3.33337 29.2 10.8 36.6666 20 36.6666C29.2 36.6666 36.6667 29.2 36.6667 20C36.6667 10.8 29.2 3.33331 20 3.33331ZM16.6667 28.3333L8.33337 20L10.6834 17.65L16.6667 23.6166L29.3167 10.9666L31.6667 13.3333L16.6667 28.3333Z"
                />
              </svg>
            </div>

            <div class="px-4 py-2 -mx-3">
              <div class="flex flex-col items-start mx-3">
                <span class="font-semibold text-[var(--primary)]">{{
                  notification.title
                }}</span>
                <p class="text-sm text-[var(--text-grey-color)]">
                  {{ notification.text }}
                </p>
              </div>
            </div>

            <!-- Close btn -->
            <button
              @click="close(notification.id)"
              class="absolute top-[-2px] right-[-5px] px-4 py-3"
              data-testid="button-close"
            >
              <svg
                class="w-4 h-4 text-[var(--text-color)] fill-current"
                role="button"
                xmlns="http://www.w3.org/2000/svg"
                viewBox="0 0 20 20"
              >
                <title>Close</title>
                <path
                  d="M14.348 14.849a1.2 1.2 0 0 1-1.697 0L10 11.819l-2.651 3.029a1.2 1.2 0 1 1-1.697-1.697l2.758-3.15-2.759-3.152a1.2 1.2 0 1 1 1.697-1.697L10 8.183l2.651-3.031a1.2 1.2 0 1 1 1.697 1.697l-2.758 3.152 2.758 3.15a1.2 1.2 0 0 1 0 1.698z"
                />
              </svg>
            </button>
          </div>
        </Notification>
      </div>
    </div>
  </NotificationGroup>
</template>
