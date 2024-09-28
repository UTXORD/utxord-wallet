import type { Manifest } from 'webextension-polyfill'
import pkg from '../package.json'
import { IS_DEV, TARGET, PORT } from '../scripts/utils'
import { LABELS, NETWORKS, NETWORK } from '~/config/index';
console.log('TARGET::',TARGET,' NETWORK::',NETWORK);

export async function getManifest(): Promise<Manifest.WebExtensionManifest> {
  // update this file to update this manifest.json
  // can also be conditional based on your need

  let permissions = [];

  switch (TARGET) {
  case '_utxord':
    permissions = [
      'https://utxord.com/*',
      'https://api.utxord.com/*',
      'https://sntry.utxord.com/*',
    ];
    break;
  case '_qa':
    permissions = [
      'https://qa.utxord.com/*',
      'https://api.qa.utxord.com/*',
      'https://sntry.utxord.com/*',
    ];
      break;
  case '_e2e':
  default:
    permissions = [
      'http://e2e.utxord.com:9000/*',
      'http://localhost/*',
      'http://127.0.0.1/*',
      'https://sntry.utxord.com/*',
    ];
}

  const manifest: Manifest.WebExtensionManifest = {
    manifest_version: 3,
    name: `${(pkg.displayName || pkg.name)} ${LABELS[TARGET]}`,
    version: pkg.version,
    description: pkg.description,
    side_panel: {
      default_path: "./popup/index.html"
    },
    action: {
      default_icon: `./assets/${NETWORKS[TARGET]}-128x128.png`,
      default_popup: './popup/index.html'
    },
    icons: {
      16: `./assets/${NETWORKS[TARGET]}-16x16.png`,
      32: `./assets/${NETWORKS[TARGET]}-32x32.png`,
      48: `./assets/${NETWORKS[TARGET]}-48x48.png`,
      128: `./assets/${NETWORKS[TARGET]}-128x128.png`
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
        matches: permissions,
        js: ['./content/index.global.js']
      }
    ],
    host_permissions: permissions,
    web_accessible_resources: [{
      resources: [],
      matches: ['<all_urls>']
    }],
    externally_connectable: {
      matches: ['<all_urls>']
    },
    // permissions: ['contextMenus', 'background', 'storage', 'nativeMessaging', 'declarativeContent', 'activeTab', 'tabs', 'scripting', 'alarms', 'unlimitedStorage'],
    permissions: ['alarms', 'scripting', 'storage', 'unlimitedStorage', 'tabs', 'activeTab','sidePanel'],
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
