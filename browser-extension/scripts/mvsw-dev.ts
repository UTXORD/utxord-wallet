import { resolve } from 'path'
import fs from 'fs-extra'
import { logger } from './utils'
;(async () => {
  try {
    // copy icon files
    await fs.copy(resolve('public'), resolve('extension/dev/assets'), {
      overwrite: true
    })
    await fs.copy(resolve('src/assets'), resolve('extension/dev/assets'), {
      overwrite: true
    })
    await fs.copy(resolve('src/libs'), resolve('extension/dev'), {
      overwrite: true
    })
    await fs.move(
      resolve('extension/dev/dist/background/index.global.js'),
      resolve('extension/dev/background.js'),
      { overwrite: true }
    )
    await fs.move(
      resolve('extension/dev/dist/content'),
      resolve('extension/dev/content'),
      { overwrite: true }
    )
    await fs.remove(resolve('extension/dev/dist'))
    // eslint-disable-next-line no-console
    logger('BUILD:SW', 'Moved service-worker success!')
  } catch (err) {
    console.error(err)
  }
})()
