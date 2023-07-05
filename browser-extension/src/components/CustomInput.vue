<template>
  <div
    class="custom-input flex flex-col items-start relative"
    :class="{ 'pb-4': rules?.length }"
  >
    <div class="relative w-full">
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
        :model-value="props.modelValue"
        @input="onInput"
        class="w-full"
        placeholder="Enter here"
        :class="{ 'custom-input--error': errors?.length }"
      />
      <div class="absolute inset-y-0 right-0 flex items-center px-2">
        <label
          v-if="props.type === TYPE_PASSWORD"
          @click="togglePassword"
          class="select-none bg-gray-300 hover:bg-gray-400 active:bg-gray-300 rounded px-2 py-1 text-sm text-gray-600 font-mono cursor-pointer js-password-label"
          for="toggle"
        >
          {{ showedPassword ? 'hide' : 'show' }}
        </label>
        <slot />
      </div>
    </div>
    <!-- Errors -->
    <div
      v-if="props.rules?.length"
      class="custom-input_error absolute bottom-[-2px] left-0"
    >
      <span v-if="errors?.length" class="text-red-300">{{ errors[0] }}</span>
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
input {
  background: #fafafa;
  border: 1px solid #ededed;
  border-radius: 4px;
  padding: 12px;
  color: #000000;
  font-size: 14px;
}

.custom-input {
  &--error {
    outline: red auto 1px !important;
  }
}
</style>
