import type { Manifest } from 'webextension-polyfill'
import pkg from '../package.json'
import { IS_DEV, PORT } from '../scripts/utils'

export async function getManifest(): Promise<Manifest.WebExtensionManifest> {
  // update this file to update this manifest.json
  // can also be conditional based on your need

  const manifest: Manifest.WebExtensionManifest = {
    manifest_version: 3,
    name: pkg.displayName || pkg.name,
    version: pkg.version,
    description: pkg.description,
    action: {
      default_icon: './assets/icon128.png',
      default_popup: './popup/index.html'
    },
    icons: {
      16: './assets/icon16.png',
      32: './assets/icon32.png',
      48: './assets/icon48.png',
      128: './assets/icon128.png'
    },
    options_ui: {
      page: './options/index.html',
      open_in_tab: true
    },
    background: {
      service_worker: 'background.js',
    },
    content_scripts: [
      {
        matches: [
            'https://utxord.com/*',
            'https://*.utxord.com/*',
            'http://localhost:*/*',
            'http://127.0.0.1:*/*',
            '<all_urls>'
        ],
        js: ['./content/index.global.js']
      }
    ],
    host_permissions: [
        'https://utxord.com/*',
        'https://*.utxord.com/*',
        'http://localhost:*/*',
        'http://127.0.0.1:*/*',
        '<all_urls>'
    ],
    web_accessible_resources: [{
      resources: [],
      matches: ['<all_urls>']
    }],
    externally_connectable: {
      matches: ['<all_urls>']
    },
    permissions: ['contextMenus', 'background', 'storage', 'nativeMessaging', 'declarativeContent', 'activeTab', 'tabs', 'scripting', 'alarms', 'unlimitedStorage'],
    permissions: ['alarms', 'scripting', 'storage', 'tabs'],
    optional_permissions: [],
    content_security_policy: {
      extension_pages: `script-src 'self' 'wasm-unsafe-eval'; object-src 'self' 'wasm-unsafe-eval'; worker-src 'self' 'wasm-unsafe-eval' http://localhost:* http://127.0.0.1:*; script-src-elem 'self' 'wasm-unsafe-eval'; connect-src * data: blob: filesystem:; style-src 'self' data: chrome-extension-resource: 'unsafe-inline'; img-src 'self' data: chrome-extension-resource:; frame-src 'self' data: chrome-extension-resource:; font-src 'self' data: chrome-extension-resource:; media-src * data: blob: filesystem:;`,
    }
  }

  if (IS_DEV) {
    // this is required on dev for Vite script to load
    manifest.content_security_policy = {
      extension_pages: `script-src 'self' http://localhost:${PORT} 'wasm-unsafe-eval'; object-src 'self'; worker-src 'self'; script-src-elem 'self' http://localhost:${PORT} 'wasm-unsafe-eval'; connect-src * data: blob: filesystem:; style-src 'self' data: chrome-extension-resource: 'unsafe-inline'; img-src 'self' data: chrome-extension-resource:; font-src 'self' data: chrome-extension-resource:; media-src * data: blob: filesystem:;`,
    }
  }

  return manifest
}
