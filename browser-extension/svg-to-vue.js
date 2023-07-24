const fs = require('fs');

function generateVueComponent(svgContent, componentName) {
  const template = `
<template>
  ${svgContent}
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
  name: '${componentName}'
});
</script>

<style lang="scss" scoped>
/* Add any custom styles for the component */
</style>
`;

  return template;
}

function convertSvgToVueComponent(svgFilePath, outputFilePath) {
  // Read the SVG file
  fs.readFile(svgFilePath, 'utf8', (err, data) => {
    if (err) {
      console.error(`Error reading SVG file: ${err}`);
      return;
    }

    const svgContent = data.trim();

    // Extract the component name from the SVG file name
    const componentName = outputFilePath
      .replace(/^.*[\\/]/, '')
      .replace(/\.[^.]*$/, '');

    // Generate the Vue component code
    const componentCode = generateVueComponent(svgContent, componentName);

    // Write the component code to the output file
    fs.writeFile(outputFilePath, componentCode, 'utf8', (err) => {
      if (err) {
        console.error(`Error writing Vue component file: ${err}`);
        return;
      }

      console.log(`SVG file converted to Vue component: ${outputFilePath}`);
    });
  });
}

// Usage: node svg-to-vue.js input.svg output.vue
if (process.argv.length !== 4) {
  console.error('Usage: node svg-to-vue.js input.svg output.vue');
  process.exit(1);
}

const inputFilePath = process.argv[2];
const outputFilePath = process.argv[3];

convertSvgToVueComponent(inputFilePath, outputFilePath);
