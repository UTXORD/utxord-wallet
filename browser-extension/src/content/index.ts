/* eslint-disable no-console */
import { onMessage } from 'webext-bridge'
import { MESSAGE_FROM_PLUGIN, MESSAGE_FROM_WEB } from "~/config/events";

// console.info('[chrome-ext-mv3-starter] Сontent script started')

// communication example: send previous tab title from background page
onMessage('tab-prev', ({ data }) => {
  console.log(`[chrome-ext-mv3-starter] Navigate from page "${data.title}"`)
})

// Handle messages from the page
window.addEventListener('message', async (event) => {
  // Check that the message is coming from the same page (not from third-party scripts)
  if (event.origin !== window.location.origin) return;

  // Check the message is from the page
  if (event.data && event.data.from === MESSAGE_FROM_WEB) {
    // Send message to Background Script
    sendMessage(event.data.type, event.data.payload, 'background');
  } else {
    // console.warn(`Unallowed message from ${event.data.from}, type: ${event.data.type}, data:`, event.data);
  }
});
