import { notify } from 'notiwind'
import { useDark, useToggle } from '@vueuse/core'
import * as webext from "webext-bridge";
import { ADDRESS_COPIED, GENERATE_MNEMONIC, VALIDATE_MNEMONIC } from '~/config/events'
import { validate, getAddressInfo } from 'bitcoin-address-validation';

export const isDark = useDark()
export const toggleDark = useToggle(isDark)
const sendMessageLimit = 5
const sendMessageTimeout = 100

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

export async function sendMessage(type, message, destination, index?: number){
  let output = null;
  if(!index){
    index = 0;
  }
  try {
    output = await webext.sendMessage(type, message, destination)
  } catch (error) {
    console.log('sendMessageError:', error)
    if(index >=  sendMessageLimit){
       console.warn('sendMessageError: limit has expired');
       return output;
     }
    return setTimeout(() => {
      index += 1
      return sendMessage(type, message, destination, index);
    }, sendMessageTimeout);
  }
  return output;
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

export async function generateMnemonic(length: number) {
  const mnemonic = await sendMessage(GENERATE_MNEMONIC, {length}, 'background');
  // console.log('helpers.generateMnemonic: mnemonic:', mnemonic)
  return mnemonic;
}

export async function isMnemonicValid(mnemonic: str) {
  const isValid = await sendMessage(VALIDATE_MNEMONIC, {mnemonic}, 'background');
  // console.log('helpers.isMnemonicValid: isValid:', isValid);
  return isValid;
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

export function toNumberFormat(num: number){
  return new Intl.NumberFormat(/* format || */ 'en-US', {
    style: 'decimal',
  }).format(num);
}

export function validateBtcAddress(address: string){
  let addressInfo = {}
  try {
    addressInfo = getAddressInfo(address)
  } catch (e) {

  }
  //Taproot and Native Segwit only
  if(addressInfo.type!=='p2tr' && addressInfo.type!=='p2wpkh'){
    return false;
  }
  return validate(address)
}

export function validateBtcAddressInfo(address: string){
  let addressInfo = {}
  let error = {}
  try {
    addressInfo = getAddressInfo(address)
  } catch (e) {
    error = e
  }
  //Taproot and Native Segwit only
  if(addressInfo.type!=='p2tr' && addressInfo.type!=='p2wpkh'){
    return `We are temporarily not supporting ${addressInfo.type}, please use p2tr and p2wpkh types`;
  }
  if(!validate(address)){
    console.log(error)
    return `Address is incorrect`
  }
}

export function detectBrowser() {
    const userAgent = navigator.userAgent.toLowerCase();
    if (userAgent.indexOf('firefox') !== -1) {
        return 'Firefox';
    } else if (userAgent.indexOf('chrome') !== -1) {
        return 'Chrome';
    } else if (userAgent.indexOf('safari') !== -1) {
        return 'Safari';
    } else if (userAgent.indexOf('opera') !== -1 || userAgent.indexOf('opr') !== -1) {
        return 'Opera';
    } else if (userAgent.indexOf('msie') !== -1 || userAgent.indexOf('trident') !== -1) {
        return 'Internet Explorer';
    } else {
        return 'Unknown';
    }
}
