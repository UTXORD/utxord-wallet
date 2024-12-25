import { sendMessage } from '~/helpers/messenger'
import { BASE_URL_PATTERN } from '~/config/index'

document.addEventListener('MESSAGE_FROM_WEB', (async (event: CustomEvent) => {
  if (event.target?.location.origin !== window.location.origin) return;

  const customEvent = event as CustomEvent;
  const message = customEvent.detail;

  // console.debug('--- content/index.ts: on MESSAGE_FROM_WEB: sendMessage:', message);
  sendMessage(message.type, message.data, 'background');

}) as EventListener);
