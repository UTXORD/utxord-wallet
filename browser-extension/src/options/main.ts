import 'vue-global-api'
import { createApp } from 'vue'
import App from '~/popup/Popup.vue'
import i18n from '~/plugins/i18n'
import '../styles'
import settingsRouter from "~/popup/settingsRouter";
import Notifications from "notiwind";
import FloatingVue from "floating-vue";
import VClickOutside from "~/popup/directives/VClickOutside";
import {createPinia} from "pinia";
import piniaPluginPersistedstate from "pinia-plugin-persistedstate";

const pinia = createPinia()
pinia.use(piniaPluginPersistedstate)

const app = createApp(App)

app
  .use(pinia)
  .use(settingsRouter)
  .use(i18n)
  .use(Notifications)
  .use(FloatingVue)
  .directive('click-outside', VClickOutside)
  .mount('#app')
