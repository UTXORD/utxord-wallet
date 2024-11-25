
import { sendMessage } from '~/helpers/index'

document.addEventListener('MESSAGE_FROM_WEB', ((event: CustomEvent) => {
  if (event.target?.location.origin !== window.location.origin) return;

  const customEvent = event as CustomEvent;
  const message = customEvent.detail;

  sendMessage(message.type, message.data, 'background');
}) as EventListener);
