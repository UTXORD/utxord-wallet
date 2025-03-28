import {createRouter, createWebHashHistory, RouteRecordRaw} from "vue-router";

export const settingsRoutes: Array<RouteRecordRaw> = [
  {
    path: '/manage',
    name: 'ManagePage',
    component: () => import('~/pages/ManagePage.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/manage-password',
    name: 'ManagePasswordPage',
    component: () => import('~/pages/ManagePasswordPage.vue'),
        meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/manage-address',
    name: 'ManageAddressPage',
    component: () => import('~/pages/ManageAddressPage.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/user-address',
    name: 'UserAddressPage',
    component: () => import('~/pages/UserAddressPage.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/manage-errors',
    name: 'ManageErrors',
    component: () => import('~/pages/ManageErrors.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/view-mode',
    name: 'ManageViewMode',
    component: () => import('~/pages/ManageViewMode.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
];

const routes: Array<RouteRecordRaw> = [
  {
    path: '/',
    redirect: '/manage',
  },
  ...settingsRoutes
];


const settingsRouter = createRouter({
  history: createWebHashHistory(),
  routes
})

export default settingsRouter
