<template>
  <NotificationGroup group="error">
    <div
        class="fixed top-30 left-0 w-full flex items-start justify-end px-4"
        data-testid="notify-error"
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
            <div class="flex items-center justify-center w-12 bg-red-500">
              <svg
                class="w-6 h-6 text-white fill-current"
                viewBox="0 0 40 40"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path
                  d="M20 3.36667C10.8167 3.36667 3.3667 10.8167 3.3667 20C3.3667 29.1833 10.8167 36.6333 20 36.6333C29.1834 36.6333 36.6334 29.1833 36.6334 20C36.6334 10.8167 29.1834 3.36667 20 3.36667ZM19.1334 33.3333V22.9H13.3334L21.6667 6.66667V17.1H27.25L19.1334 33.3333Z"
                />
              </svg>
            </div>

            <div class="px-4 py-2 -mx-3">
              <div class="flex flex-col items-start mx-3">
                <span class="font-semibold text-[var(--error)]">{{
                  notification.title
                }}</span>
                <p class="text-sm text-left text-[var(--text-grey-color)]">
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
