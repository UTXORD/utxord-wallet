import { notify } from 'notiwind'
import { useDark, useToggle } from '@vueuse/core'

export const isDark = useDark()
export const toggleDark = useToggle(isDark)

export function showSuccess(title: string, text: string) {
  notify({
    group: 'success',
    title,
    text
  }, 4000) // 4s
}

export function showError(title: string, text: string) {
  notify({
    group: 'error',
    title,
    text
  }, 10000) // 10s
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

export function copyToClipboard(text: string) {
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

    showSuccess('Success', 'Address was copied!');
    return text;
  }
  return null;
}

export function isASCII(str: string) {
  return /^[\x00-\x7F]*$/.test(str);
}
