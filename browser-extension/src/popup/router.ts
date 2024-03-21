import { createRouter, createWebHashHistory, RouteRecordRaw } from 'vue-router'

const START_ROUTE = {
  path: '/start',
  name: 'StartScreen',
  component: () => import('~/components/StartScreen.vue')
}

const routes: Array<RouteRecordRaw> = [
  START_ROUTE,
  {
    path: '/',
    name: 'HomeScreen',
    component: () => import('~/components/HomeScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/export-keys',
    name: 'ExportKeys',
    component: () => import('~/components/ExportKeysScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-transfer-collection',
    name: 'SignTransferScreen',
    component: () => import('~/components/SignTransferScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-create-inscription',
    name: 'SignCreateScreen',
    component: () => import('~/components/SignCreateScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-sell',
    name: 'SignSellScreen',
    component: () => import('~/components/SignSellScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-commit-buy',
    name: 'SignCommitScreen',
    component: () => import('~/components/SignCommitScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/sign-buy',
    name: 'SignBuyScreen',
    component: () => import('~/components/SignBuyScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/manage',
    name: 'ManageScreen',
    component: () => import('~/components/ManageScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/manage-password',
    name: 'ManagePasswordScreen',
    component: () => import('~/components/ManagePasswordScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/manage-address',
    name: 'ManageAddressScreen',
    component: () => import('~/components/ManageAddressScreen.vue'),
    meta: {
      requiresAuth: true
    }
  },
  {
    path: '/create-password-screen',
    name: 'CreatePasswordScreen',
    component: () => import('~/components/CreatePasswordScreen.vue')
  },
  {
    path: '/generate',
    name: 'GenerateScreen',
    component: () => import('~/components/GenerateScreen.vue')
  },
  {
    path: '/load',
    name: 'LoadScreen',
    component: () => import('~/components/LoadScreen.vue')
  },
  {
    path: '/alert-mnemonic',
    name: 'AlertMnemonic',
    component: () => import('~/components/AlertMnemonic.vue')
  },
  {
    path: '/wallet-created',
    name: 'WalletCreated',
    component: () => import('~/components/WalletCreated.vue')
  },
  {
    path: '/loading',
    name: 'LoadingScreen',
    component: () => import('~/components/LoadingScreen.vue')
  },
]

const router = createRouter({
  history: createWebHashHistory(),
  routes
})

router.beforeEach((to, from, next) => {
  const tempMnemonic = localStorage?.getItem('temp-mnemonic')
  console.log(tempMnemonic);
  
  if (!tempMnemonic && to.path !== START_ROUTE.path && to.matched.some((record) => record.meta.requiresAuth)) {
    next({ path: START_ROUTE.path })
  } else {
    next()
  }
})

export default router
