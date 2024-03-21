import { createRouter, createWebHashHistory, RouteRecordRaw } from 'vue-router'

const routes: Array<RouteRecordRaw> = [
  {
    path: '/',
    name: 'HomeScreen',
    component: () => import('~/components/HomeScreen.vue')
  },
  {
    path: '/start',
    name: 'StartScreen',
    component: () => import('~/components/StartScreen.vue')
  },
  {
    path: '/create-password-screen',
    name: 'CreatePasswordScreen',
    component: () => import('~/components/CreatePasswordScreen.vue')
  },
  {
    path: '/export-keys',
    name: 'ExportKeys',
    component: () => import('~/components/ExportKeysScreen.vue')
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
  {
    path: '/sign-transfer-collection',
    name: 'SignTransferScreen',
    component: () => import('~/components/SignTransferScreen.vue')
  },
  {
    path: '/sign-create-inscription',
    name: 'SignCreateScreen',
    component: () => import('~/components/SignCreateScreen.vue')
  },
  {
    path: '/sign-sell',
    name: 'SignSellScreen',
    component: () => import('~/components/SignSellScreen.vue')
  },
  {
    path: '/sign-commit-buy',
    name: 'SignCommitScreen',
    component: () => import('~/components/SignCommitScreen.vue')
  },
  {
    path: '/sign-buy',
    name: 'SignBuyScreen',
    component: () => import('~/components/SignBuyScreen.vue')
  },
  {
    path: '/manage',
    name: 'ManageScreen',
    component: () => import('~/components/ManageScreen.vue')
  },
  {
    path: '/manage-password',
    name: 'ManagePasswordScreen',
    component: () => import('~/components/ManagePasswordScreen.vue')
  },
  {
    path: '/manage-address',
    name: 'ManageAddressScreen',
    component: () => import('~/components/ManageAddressScreen.vue')
  },
]

const router = createRouter({
  history: createWebHashHistory(),
  routes
})

export default router
