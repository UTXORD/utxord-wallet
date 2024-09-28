import 'vue-global-api'
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import Notifications from 'notiwind'
import piniaPluginPersistedstate from 'pinia-plugin-persistedstate'
import App from './Popup.vue'
import i18n from '~/plugins/i18n'
import router from './router'
import '../styles'
import VClickOutside from './directives/VClickOutside'
import FloatingVue from 'floating-vue'
import * as Sentry from '@sentry/vue'
import 'floating-vue/dist/style.css'
import {
  BASE_URL_PATTERN,
  PROD_URL_PATTERN,
  STAGE_URL_PATTERN,
  ETWOE_URL_PATTERN,
  LOCAL_URL_PATTERN,
  NETWORK,
  TESTNET,
  SIGNET,
  REGTEST,
  MAINNET
 } from '~/config/index';
 import {version} from '~/../package.json';
 import { STATUS_VIEW_MODE  } from '~/config/events';
 import { sendMessage } from 'webext-bridge'


const pinia = createPinia()
pinia.use(piniaPluginPersistedstate)

const app = createApp(App)

async function ViewMode() {
  const res = await sendMessage(STATUS_VIEW_MODE, {}, 'background')
  chrome.sidePanel
    .setPanelBehavior({ openPanelOnActionClick: res.viewMode })
    .catch((error) => console.error(error));
}
ViewMode();



Sentry.init({
  app,
  environment: () => {
    switch (BASE_URL_PATTERN) {
      case PROD_URL_PATTERN: return 'production'
      case STAGE_URL_PATTERN: return 'staging'
      case ETWOE_URL_PATTERN: return 'staging'
      case LOCAL_URL_PATTERN: return 'debuging'
      default: return 'debuging'
    }
  },
  network: () => {
    switch (NETWORK) {
      case SIGNET: return 'signet'
      case TESTNET: return 'testnet'
      case REGTEST: return 'regtest'
      case MAINNET: return 'mainnet'
      default: return 'mainnet'
    }
  },
  plugin: version,
  dsn: "https://9b55ac2faadbc147285ed63295e018ea@sntry.utxord.com/4",
  integrations: [
    Sentry.browserTracingIntegration({ router }),
    Sentry.replayIntegration(),
  ],

  // Set tracesSampleRate to 1.0 to capture 100%
  // of transactions for performance monitoring.
  // We recommend adjusting this value in production
  tracesSampleRate: 1.0,

  // Set `tracePropagationTargets` to control for which URLs distributed tracing should be enabled
  tracePropagationTargets: ["localhost", /^https:\/\/yourserver\.io\/api/],

  // Capture Replay for 10% of all sessions,
  // plus for 100% of sessions with an error
  replaysSessionSampleRate: 0.1,
  replaysOnErrorSampleRate: 1.0,
});
app
  .use(pinia)
  .use(router)
  .use(i18n)
  .use(Notifications)
  .use(FloatingVue)
  .directive('click-outside', VClickOutside)
  .mount('#app')
