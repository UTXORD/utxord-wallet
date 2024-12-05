import { sendMessage } from '~/helpers/messenger'
import { BASE_URL_PATTERN } from '~/config/index'
document.addEventListener('MESSAGE_FROM_WEB', (async (event: CustomEvent) => {
  console.log('MESSAGE_FROM_WEB:',event);
  if (event.target?.location.origin !== window.location.origin) return;

  const customEvent = event as CustomEvent;
  const message = customEvent.detail;

  sendMessage(message.type, message.data, 'background');

  const base_url = BASE_URL_PATTERN.replace('*', '');
  const tabs = await browser.tabs.query({ currentWindow: true });
  for (let tab of tabs) {
    const url = tab?.url || tab?.pendingUrl;
    if(tab?.id &&
       url?.startsWith(base_url) &&
       !url?.startsWith('about:') &&
       !url?.startsWith('browser-extension://') &&
       !url?.startsWith('chrome://')) {
      await browser.scripting.executeScript({
        target: { tabId: tab?.id },
        func: () =>window.localStorage.removeItem('MESSAGE_FROM_WEB'),
        args: [],
      });
    }
  }

}) as EventListener);
