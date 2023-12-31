import { resolve } from 'path'
import fs from 'fs-extra'
import { logger, r, re_pre_config } from './utils'

;(async () => {
  try {

    // move icon files
    await fs.copy(resolve('public'), resolve('extension/prod/assets'), {
      overwrite: true
    })
    await fs.copy(resolve('src/libs'), resolve('extension/prod'), {
      overwrite: true
    })

    // move service worker
    await fs.move(
      resolve('extension/prod/dist/background/index.global.js'),
      resolve('extension/prod/background.js'),
      { overwrite: true }
    )
    await fs.move(
      resolve('extension/prod/dist/content'),
      resolve('extension/prod/content'),
      { overwrite: true }
    )
    await fs.remove(resolve('extension/prod/dist'))
    // eslint-disable-next-line no-console
    logger('BUILD:SW', 'Moved service-worker success!')
  } catch (err) {
    await re_pre_config()
    console.error(err)
  }
  await re_pre_config()

})()
