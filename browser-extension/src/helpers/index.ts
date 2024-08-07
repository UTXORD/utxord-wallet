import { notify } from 'notiwind'
import { useDark, useToggle } from '@vueuse/core'
import {sendMessage} from "webext-bridge";
import {ADDRESS_COPIED} from "~/config/events";
import * as WebBip39 from 'web-bip39';
import wordlist from 'web-bip39/wordlists/english';

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
  if (address?.length) {
    return (
      address.substring(0, (start || 3)) +
      '...' +
      address.substring(
        address?.length - (end || 3),
        address?.length
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

export async function isMnemonicValid(val: string){
 const valid = await WebBip39.validateMnemonic(val, wordlist)
 return valid
}

export function isContains(str: string) {
  const containsUppercase = /[A-Z]/.test(str)
  const containsLowercase = /[a-z]/.test(str)
  const containsNumber = /[0-9]/.test(str)
  const containsSpecial = /[#?!@$%^&*-\|\.;\'\"\`,\\/\>\<\[\]{}()]/.test(str)
return containsUppercase && containsLowercase && containsNumber && containsSpecial;
}

export function getRandom(mins:number = 0, maxs:number = 2147483647): number{
  return Math.floor(Math.random() * (maxs - mins + 1)) + mins;
}

export function isLength(str: string) {
  return (str?.length > 8);
}

export function convertSatsToUSD(sats: number, usdRate: number): number {
  return (sats || 0) * (1 / 100000000) * (usdRate || 0);
}
