<template>
  <div>
    <slot name="button" :onClick="open" />
    <div
      v-if="opened"
      tabindex="-1"
      aria-hidden="true"
      class="absolute top-0 left-0 right-0 z-50 w-full p-4 overflow-x-hidden overflow-y-auto md:inset-0 max-h-full h-full bg-black bg-opacity-20"
    >
      <div class="absolute bottom-0 left-0 w-full max-w-2xl max-h-full">
        <!-- Modal content -->
        <div
          class="relative bg-[var(--bg-color)] rounded-tl-xl rounded-tr-xl shadow"
        >
          <!-- Modal header -->
          <div
            class="flex items-center justify-between p-4 border-b border-[var(--border-color)] rounded-t"
          >
            <div class="text-lg font-semibold text-[var(--text-color)]">
              {{ props.title }}
            </div>
            <button
              type="button"
              class="text-gray-400 bg-transparent hover:bg-gray-200 hover:text-gray-900 rounded-lg text-sm p-1.5 ml-auto inline-flex items-center"
              @click="close"
            >
              <svg
                aria-hidden="true"
                class="w-5 h-5"
                fill="currentColor"
                viewBox="0 0 20 20"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path
                  fill-rule="evenodd"
                  d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z"
                  clip-rule="evenodd"
                ></path>
              </svg>
            </button>
          </div>
          <!-- Modal body -->
          <div class="p-4 space-y-6">
            <slot name="body" />
          </div>
          <!-- Modal footer -->
          <div class="w-full min-h-[1px] bg-[var(--border-color)]" />
          <div class="flex items-center p-4 space-x-2 rounded-b">
            <Button @click="close" outline class="w-2/4">
              {{ props.cancelText }}
            </Button>
            <Button :disabled="props.disabled" @click="submit" class="w-2/4">{{
              props.submitText
            }}</Button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, defineProps, defineEmits, onMounted, onUnmounted } from 'vue'
import Button from '~/components/Button.vue'

const opened = ref(false)

const props = defineProps({
  title: {
    type: String,
    default: 'Modal Title'
  },
  disabled: {
    type: Boolean,
    default: false
  },
  submitByEnter: {
    type: Boolean,
    default: false
  },
  submitText: {
    type: String,
    default: 'Submit'
  },
  cancelText: {
    type: String,
    default: 'Cancel'
  }
})

const emit = defineEmits(['on-submit', 'on-close'])

function open() {
  opened.value = true
}

function close() {
  opened.value = false
  emit('on-close')
}

function submit() {
  emit('on-submit')
  close()
}

function onKeyDown(e) {
  if (e.key === 'Enter' && e.target.value) {
    submit()
  }
  if (e.key === 'Escape') {
    close()
  }
}

onMounted(() => {
  if (props.submitByEnter) {
    window.addEventListener('keydown', onKeyDown)
  }
})

onUnmounted(() => {
  if (props.submitByEnter) {
    window.removeEventListener('keydown', onKeyDown)
  }
})
</script>
