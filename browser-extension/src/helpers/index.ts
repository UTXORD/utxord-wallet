import { notify } from 'notiwind'
import { useDark, useToggle } from '@vueuse/core'
import {sendMessage} from "webext-bridge";
import {ADDRESS_COPIED} from "~/config/events";

export const isDark = useDark()
export const toggleDark = useToggle(isDark)

export function showSuccess(title: string, text: string, duration: number = 4000) { // 4s
  notify({
    group: 'success',
    title,
    text
  }, duration)
}

export function showError(title: string, text: string, duration: number = 10000) { // 10s
  notify({
    group: 'error',
    title,
    text
  }, duration)
}

export function formatAddress(address: string, start?: number, end?: number) {
  if (address) {
    return (
      address.substring(0, (start || 3)) +
      '...' +
      address.substring(
        address.length - (end || 3),
        address.length
      )
    );
  }
  return '-';
}

export function copyToClipboard(text: string, message?: string) {
  if (text) {
    const tempInput = document.createElement('input');
    tempInput.style.position = 'absolute';
    tempInput.style.top = '0';
    tempInput.style.left = '0';
    document.body.appendChild(tempInput);
    tempInput.value = text;
    tempInput.select();
    tempInput.setSelectionRange(0, 99999); /* For mobile devices */
    document.execCommand('copy');
    document.body.removeChild(tempInput);

    sendMessage(ADDRESS_COPIED, {}, 'background')
    showSuccess('Success', message || 'Address was copied!');
    return text;
  }
  return null;
}

export function isASCII(str: string) {
  return /^[\x00-\x7F]*$/.test(str);
}
