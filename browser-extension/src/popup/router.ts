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
    path: '/password',
    name: 'PasswordScreen',
    component: () => import('~/components/PasswordScreen.vue')
  },
]

const router = createRouter({
  history: createWebHashHistory(),
  routes
})

export default router
