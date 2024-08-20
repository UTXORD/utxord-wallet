import { createRouter, createWebHashHistory, RouteRecordRaw } from 'vue-router'
import {sendMessage} from 'webext-bridge';
import {CHECK_AUTH, CURRENT_PAGE} from '~/config/events';
import {settingsRoutes} from "~/popup/settingsRouter";

const START_ROUTE = {
  path: '/start',
  name: 'StartPage',
  component: () => import('~/pages/StartPage.vue')
}

const routes: Array<RouteRecordRaw> = [
  START_ROUTE,
  {
    path: '/',
    name: 'HomePage',
    component: () => import('~/pages/HomePage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/export-keys',
    name: 'ExportKeysPage',
    component: () => import('~/pages/ExportKeysPage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-transfer-collection',
    name: 'SignTransferPage',
    component: () => import('~/pages/SignTransferPage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-create-inscription',
    name: 'SignCreatePage',
    component: () => import('~/pages/SignCreatePage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-buy-product',
    name: 'SignBuyProductPage',
    component: () => import('~/pages/SignBuyProductPage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-sell',
    name: 'SignSellPage',
    component: () => import('~/pages/SignSellPage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/estimate-fee',
    name: 'EstimateFeePage',
    component: () => import('~/pages/EstimateFeePage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-commit-buy',
    name: 'SignCommitPage',
    component: () => import('~/pages/SignCommitPage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-buy',
    name: 'SignBuyPage',
    component: () => import('~/pages/SignBuyPage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/create-password-screen',
    name: 'CreatePasswordPage',
    component: () => import('~/pages/CreatePasswordPage.vue')
},
  {
    path: '/generate',
    name: 'GeneratePage',
    component: () => import('~/pages/GeneratePage.vue')
  },
  {
    path: '/check-user-mnemonic',
    name: 'checkUserMnemonic',
    component: () => import('~/pages/checkUserMnemonic.vue')
  },
  {
    path: '/load',
    name: 'LoadPage',
    component: () => import('~/pages/LoadPage.vue')
  },
  {
    path: '/alert-mnemonic',
    name: 'AlertMnemonicPage',
    component: () => import('~/pages/AlertMnemonicPage.vue')
  },
  {
    path: '/wallet-created',
    name: 'WalletCreatedPage',
    component: () => import('~/pages/WalletCreatedPage.vue')
  },
  {
    path: '/loading',
    name: 'LoadingPage',
    component: () => import('~/pages/LoadingPage.vue')
  },
  ...settingsRoutes
]

const router = createRouter({
  history: createWebHashHistory(),
  routes
})

router.beforeEach(async (to, from, next) => {
  let authenticated = false;
  try {
    authenticated = await sendMessage(CHECK_AUTH, {}, 'background');
  } catch(e) {}
  const currentPage = await localStorage?.getItem(CURRENT_PAGE)
  const pageMatched = to.matched.some(record => record.meta.requiresAuth)
  console.log('authenticated:', authenticated, ' to.path:', to.path);
  if (!authenticated && pageMatched) {
    if(!currentPage) next({ path: START_ROUTE.path });
    if(to.path === '/' && currentPage === START_ROUTE.path) next({ path: START_ROUTE.path });
    if(currentPage === '/') next({ path: START_ROUTE.path });
    if(currentPage !=='/' && currentPage !== START_ROUTE.path){
      next({ path: currentPage });
    }
  }
  await localStorage?.setItem(CURRENT_PAGE, to.path);
  next();
})

export default router
