import * as CryptoJS from 'crypto-js'

export const CURRENT_PAGE = 'current-page'
export const PASSWORD_VALUE = 'temp-password-value'
export const PASSWORD_CONFIRM_VALUE = 'temp-password-confirm-value'
export const PASSWORD_HAS_BEEN_SET = 'temp-password-has-been-set'
export const MNEMONIC = 'temp-mnemonic'
export const MNEMONIC_LANGUAGE = 'temp-mnemonic-language'
export const MNEMONIC_LENGTH = 'temp-mnemonic-length'
export const PASSPHRASE = 'temp-passphrase'
export const WORDS_COUNT = 'temp-words-count'
export const ROW_POSITION = 'temp-row-position'

export const PUBLIC_KEY = 'publickey'
export const DARK_THEME = 'dark-theme'

const VALUE_PREFIX = 'encrypted-value-';

export function clear(): void {
    window.localStorage?.clear();
}

export function setItem(key: string, value: string, secret: string | null | undefined = undefined): void {
    if (!key) return;
    if (!secret) {
        window.localStorage?.setItem(key, value);
        return;
    }
    const iv = CryptoJS.lib.WordArray.random(128 / 8);
    const encryptedValue = CryptoJS.AES.encrypt(VALUE_PREFIX + value, secret, {
        iv: iv,
        padding: CryptoJS.pad.Pkcs7,
        mode: CryptoJS.mode.CBC,
        hasher: CryptoJS.algo.SHA256
    });
    window.localStorage?.setItem(key, encryptedValue);
}

export function getItem(key: string, secret: string | null | undefined = undefined): string | null {
    if (!key) return null;
    if (!secret) {
        return window.localStorage?.getItem(key);
    }

    const encryptedValue = window.localStorage?.getItem(key);
    if (encryptedValue) {
        const iv = CryptoJS.lib.WordArray.random(128 / 8);
        const decryptedValue = CryptoJS.AES.decrypt(encryptedValue, secret, {
            iv: iv,
            padding: CryptoJS.pad.Pkcs7,
            mode: CryptoJS.mode.CBC,
            hasher: CryptoJS.algo.SHA256
        })
        const prefixedValue = decryptedValue.toString(CryptoJS.enc.Utf8);
        if (prefixedValue.startsWith(VALUE_PREFIX)) {
            return prefixedValue.substring(VALUE_PREFIX.length);
        }
    }
    return null;
}

export function getItems(keys: string[], secret: string | null | undefined = undefined): (string | null)[] {
    return keys.map(k => getItem(k, secret));
}

export function removeItem(key: string): void {
    window.localStorage?.removeItem(key);
}

export function removeItems(keys: string[]): void {
    key.forEach(k => removeItem(k));
}

export function setItems(values: { [key: string]: string }, secret: string | null | undefined = undefined): void {
    for (const [k, v] of Object.entries(values)) {
        setItem(k, v, secret);
    }
}