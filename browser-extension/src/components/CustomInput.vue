<template>
  <div
    class="custom-input flex flex-col items-start relative"
    :class="{ 'pb-4': rules?.length && !errors?.length }"
  >
    <textarea
      ref="customInput"
      v-if="type === 'textarea'"
      v-bind="$attrs"
      :value="props.modelValue"
      @input="onInput"
      class="w-full bg-[var(--bg-color)] text-[var(--text-color)]"
      :placeholder="placeholder"
    ></textarea>
    <div class="relative w-full" v-else>
      <input
        ref="customInput"
        v-bind="$attrs"
        :type="
          props.type === TYPE_PASSWORD
            ? showedPassword
              ? TYPE_TEXT
              : TYPE_PASSWORD
            : props.type
        "
        :value="props.modelValue"
        :model-value="props.modelValue"
        @input="onInput"
        class="w-full bg-[var(--bg-color)] text-[var(--text-color)]"
        :placeholder="placeholder"
        :class="{ 'custom-input--error': errors?.length }"
      />
      <div class="absolute inset-y-0 right-0 flex items-center px-2">
        <label
          v-if="props.type === TYPE_PASSWORD"
          @click="togglePassword"
          class="select-none rounded px-2 py-2 text-sm text-gray-600 font-mono cursor-pointer js-password-label"
          for="toggle"
        >
          <EyeIcon :opened="showedPassword" class="opacity-70" />
        </label>
        <slot />
      </div>
    </div>
    <!-- Errors -->
    <div
      v-if="errors?.length"
      class="custom-input_error"
      :class="{ 'my-2': props.rules?.length }"
    >
      <span v-if="errors?.length" class="text-red-300 text-left">{{ errors[0] }}</span>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { defineProps, defineEmits, ref, onMounted } from 'vue'

const TYPE_PASSWORD = 'password'
const TYPE_TEXT = 'text'

const props = defineProps({
  modelValue: {
    type: String,
    default: null
  },
  rules: {
    type: Array,
    default: () => []
  },
  type: {
    type: String,
    default: 'text'
  },
  placeholder: {
    type: String,
    default: 'Enter here'
  },
  autofocus: {
    type: Boolean,
    default: false
  }
})
const emit = defineEmits(['update:model-value'])

const showedPassword = ref(false)
const customInput = ref(null)
const errors = ref<string[] | boolean[] | unknown[]>([])

function validateRules(val: string) {
  const rulesResult = props.rules.map((r) => r(val))
  errors.value = rulesResult.filter((r) => typeof r === 'string')
}

function onInput(event: any) {
  const value = event.target.value
  validateRules(value)
  emit('update:model-value', value)
}

function togglePassword() {
  showedPassword.value = !showedPassword.value
}

onMounted(() => {
  if (props.autofocus) {
    customInput.value?.focus()
  }
})
</script>

<style lang="scss" scoped>
input,
textarea {
  border-radius: 10px;
  padding: 12px;
  font-size: 14px;
}

.custom-input {
  &--error {
    outline: none;
    box-shadow: 0 0 0 2px red;
  }
}
</style>
