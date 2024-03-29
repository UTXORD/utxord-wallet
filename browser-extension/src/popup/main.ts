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

const pinia = createPinia()
pinia.use(piniaPluginPersistedstate)

const app = createApp(App)

Sentry.init({
  app,
  dsn: "https://9b55ac2faadbc147285ed63295e018ea@sntry.l15.co/4",
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
