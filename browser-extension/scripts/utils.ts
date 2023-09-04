import { resolve } from 'path'
import { bgCyan, black } from 'kolorist'
import fs from 'fs-extra'

export const PORT = parseInt(process.env.PORT || '') || 3309
export const r = (...args: string[]) => resolve(__dirname, '..', ...args)
export const IS_DEV = process.env.NODE_ENV !== 'production'
export const TARGET = `_${(process.env.TARGET_ENV || 'qa')}`

export function logger(name: string, message: string) {
  // eslint-disable-next-line no-console
  console.log(black(bgCyan(` ${name} `)), message)
}
export async function pre_config() {
  const confins = [
    'src/background/api.ts',
    'src/background/index.ts',
    'src/background/rest.ts',
  ];
  for(const item of confins){
    let data = await fs.readFile(r(item), 'utf-8')
        data = data
          .replace('~/config/index', `~/config/index${TARGET}`)
        await fs.writeFile(r(item), data, 'utf-8')
    }
}

export async function re_pre_config() {
  const confins = [
    'src/background/api.ts',
    'src/background/index.ts',
    'src/background/rest.ts',
  ];
  for(const item of confins){
    let data = await fs.readFile(r(item), 'utf-8')
        data = data
          .replace(`~/config/index${TARGET}`,'~/config/index')
        await fs.writeFile(r(item), data, 'utf-8')
    }
}
