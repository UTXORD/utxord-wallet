<template>
  <svg
    id="Layer_1"
    data-name="Layer 1"
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 512 512"
    class="svg-icon"
  >
    <title>refresh-glyph</title>
    <path
      d="M66.71,287.91H0v160l47.36-43.59A255.71,255.71,0,0,0,255.9,512C386.54,512,494.19,414.15,510,287.91H445.29A192,192,0,0,1,95,360.6l79.08-72.69Zm0,0"
      fill="#000"
    />
    <path
      d="M255.9,0C125.46,0,17.64,97.63,2,224H66.71a192,192,0,0,1,353.26-68l-68,68H512v-160l-45.78,45.78A255.89,255.89,0,0,0,255.9,0Zm0,0"
      fill="#000"
    />
  </svg>
</template>

<script lang="ts">
import { defineComponent } from 'vue'

export default defineComponent({
  name: 'RefreshIcon'
})
</script>

<style lang="scss" scoped>
.dark {
  .svg-icon {
    path {
      fill: #fff;
    }
  }
}
</style>
