import { createRouter, createWebHashHistory, RouteRecordRaw } from 'vue-router'
import {sendMessage} from 'webext-bridge';
import {CHECK_AUTH} from '~/config/events';

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
    path: '/sign-sell',
    name: 'SignSellPage',
    component: () => import('~/pages/SignSellPage.vue'),
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
    path: '/manage',
    name: 'ManagePage',
    component: () => import('~/pages/ManagePage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/manage-password',
    name: 'ManagePasswordPage',
    component: () => import('~/pages/ManagePasswordPage.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/manage-address',
    name: 'ManageAddressPage',
    component: () => import('~/pages/ManageAddressPage.vue'),
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
]

const router = createRouter({
  history: createWebHashHistory(),
  routes
})

router.beforeEach(async (to, from, next) => {
  const authenticated = await sendMessage(CHECK_AUTH, {}, 'background');
  console.log('authenticated:', authenticated, 'to.path:', to.path);

  if (!authenticated && to.path !== START_ROUTE.path && to.matched.some((record) => record.meta.requiresAuth)) {
    next({ path: START_ROUTE.path });
  } else {
    next();
  }
})

export default router
