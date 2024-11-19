import chineseTraditional from "~/config/bip39_wordlists/chinese-traditional.js";

export const MNEMONIC_LENGTHS = [12, 15, 18, 21, 24]
export const MNEMONIC_DEFAULT_LENGTH = MNEMONIC_LENGTHS[0]
export const MNEMONIC_LENGTH_OPTIONS = MNEMONIC_LENGTHS.map(l => ({label: l.toString(), value: l}))

import englishWords from '~/config/bip39_wordlists/english.js'
import spanishWords from '~/config/bip39_wordlists/spanish.js'
import frenchWords from '~/config/bip39_wordlists/french.js'
import italianWords from '~/config/bip39_wordlists/italian.js'
import portugueseWords from '~/config/bip39_wordlists/portuguese.js'
import czechWords from '~/config/bip39_wordlists/czech.js'
import chineseSimplifiedWords from '~/config/bip39_wordlists/chinese-simplified.js'
import chineseTraditionalWords from '~/config/bip39_wordlists/chinese-traditional.js'
import japaneseWords from '~/config/bip39_wordlists/japanese.js'
import koreanWords from '~/config/bip39_wordlists/korean.js'


const MNEMONIC_LANGUAGES = [
  'english',
  'spanish',
  'french',
  'italian',
  'portuguese',
  'czech',
  'chinese-simplified',
  'chinese-traditional',
  'japanese',
  'korean'
]
export const MNEMONIC_DEFAULT_LANGUAGE = MNEMONIC_LANGUAGES[0]
export const MNEMONIC_LANGUAGE_OPTIONS = MNEMONIC_LANGUAGES.map(l => ({
  label: (l[0].toUpperCase() + l.slice(1)).replace('-', ' '),
  value: l
}))

const BIP39_WORDS = {
  'english': englishWords,
  'spanish': spanishWords,
  'french': frenchWords,
  'italian': italianWords,
  'portuguese': portugueseWords,
  'czech': czechWords,
  'chinese-simplified': chineseSimplifiedWords,
  'chinese-traditional': chineseTraditionalWords,
  'japanese': japaneseWords,
  'korean': koreanWords
}

export function getWordlistFor(language: string) {
  if (!language) language = MNEMONIC_DEFAULT_LANGUAGE;
  return BIP39_WORDS[language];
}
