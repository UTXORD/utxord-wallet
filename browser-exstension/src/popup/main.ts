import 'vue-global-api'
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import Notifications from 'notiwind'
import piniaPluginPersistedstate from 'pinia-plugin-persistedstate'
import App from './Popup.vue'
import i18n from '~/plugins/i18n'
import router from './router'
import '../styles'

const pinia = createPinia()
pinia.use(piniaPluginPersistedstate)

const app = createApp(App)
app
  .use(pinia)
  .use(router)
  .use(i18n)
  .use(Notifications)
  .mount('#app')
