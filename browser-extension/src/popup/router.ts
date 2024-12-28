import { createRouter, createWebHashHistory, RouteRecordRaw } from 'vue-router'
import {sendMessage} from '~/helpers/index'
import {CHECK_AUTH, CURRENT_PAGE} from '~/config/events';
import {settingsRoutes} from "~/popup/settingsRouter";

const START_ROUTE = {
  path: '/start',
  name: 'StartPage',
  component: () => import('~/pages/StartPage.vue')
}
let refreshIndex = 0;
let redirectIndex = 0;
const routes: Array<RouteRecordRaw> = [
  START_ROUTE,
  {
    path: '/',
    name: 'HomePage',
    component: () => import('~/pages/HomePage.vue'),
    meta: {
      requiresAuth: true,
      restore: false
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
    path: '/send-to',
    name: 'SendPage',
    component: () => import('~/pages/SendPage.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/confirm-send-to',
    name: 'ConfirmSendPage',
    component: () => import('~/pages/ConfirmSendPage.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/sign-create-inscription',
    name: 'SignCreatePage',
    component: () => import('~/pages/SignCreatePage.vue'),
    meta: {
      requiresAuth: true,
      restore: false,
      type: 'popup'
    }
  },
  {
    path: '/sign-buy-product',
    name: 'SignBuyProductPage',
    component: () => import('~/pages/SignBuyProductPage.vue'),
    meta: {
      requiresAuth: true,
      restore: false,
      type: 'popup'
    }
  },
  {
    path: '/sign-sell',
    name: 'SignSellPage',
    component: () => import('~/pages/SignSellPage.vue'),
    meta: {
      requiresAuth: true,
      restore: false,
      type: 'popup'
    }
  },
  {
    path: '/estimate-fee',
    name: 'EstimateFeePage',
    component: () => import('~/pages/EstimateFeePage.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/addresses',
    name: 'AddressesPage',
    component: () => import('~/pages/AddressesPage.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/utxos',
    name: 'UtxosPage',
    component: () => import('~/pages/UtxosPage.vue'),
    meta: {
      requiresAuth: true,
      restore: true
    }
  },
  {
    path: '/sign-commit-buy',
    name: 'SignCommitPage',
    component: () => import('~/pages/SignCommitPage.vue'),
    meta: {
      requiresAuth: true,
      restore: false,
      type: 'popup'
    }
  },
  {
    path: '/sign-buy',
    name: 'SignBuyPage',
    component: () => import('~/pages/SignBuyPage.vue'),
    meta: {
      requiresAuth: true,
      restore: false,
      type: 'popup'
    }
  },
  {
    path: '/create-password-screen',
    name: 'CreatePasswordPage',
    component: () => import('~/pages/CreatePasswordPage.vue'),
    meta: {
      restore: true
    }
},
  {
    path: '/generate',
    name: 'GeneratePage',
    component: () => import('~/pages/GeneratePage.vue'),
    meta: {
      restore: true
    }
  },
  {
    path: '/check-user-mnemonic',
    name: 'checkUserMnemonic',
    component: () => import('~/pages/checkUserMnemonic.vue'),
    meta: {
      restore: true
    }
  },
  {
    path: '/load',
    name: 'LoadPage',
    component: () => import('~/pages/LoadPage.vue'),
    meta: {
      restore: false
    }
  },
  {
    path: '/alert-mnemonic',
    name: 'AlertMnemonicPage',
    component: () => import('~/pages/AlertMnemonicPage.vue'),
    meta: {
      restore: true
    }
  },
  {
    path: '/wallet-created',
    name: 'WalletCreatedPage',
    component: () => import('~/pages/WalletCreatedPage.vue'),
    meta: {
      restore: true
    }
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

  const authRequest = await sendMessage(CHECK_AUTH, {}, 'background');
  console.log('authRequest:',authRequest);
  const authenticated = Number.isInteger(authRequest)?false:authRequest;

  const storePage = await localStorage?.getItem(CURRENT_PAGE);
  const currentPage = to.path;

  const currentRecord = await routes.find((record) => record.path === currentPage)
  const storeRecord = await routes.find((record) => record.path === storePage)

  const currentTypePage = currentRecord?.meta?.type;
  const storeTypePage = storeRecord?.meta?.type;

  const currentRequiresAuth = Boolean(currentRecord?.meta?.requiresAuth)
  const currentRestore = Boolean(currentRecord?.meta?.restore)

  const storeRequiresAuth = Boolean(storeRecord?.meta?.requiresAuth)
  const storeRestore = Boolean(storeRecord?.meta?.restore)

  if(currentPage === storePage){
    refreshIndex += 1;
    redirectIndex = 0;
  }else{
    refreshIndex = 0;
    redirectIndex +=1;
  }
  console.log('------------------------------------------------------------------');
  console.log('currentPage:',currentPage, ' currentRecord:',currentRecord)
  console.log('storePage:',storePage, ' storeRecord:',storeRecord)
  console.log('authenticated:', authenticated, ' refreshIndex:', refreshIndex,' redirectIndex:', redirectIndex);
  console.log('currentPage:', currentPage,' currentRestore:',currentRestore,' currentTypePage:',currentTypePage);
  console.log('storePage:', storePage,' storeRestore:',storeRestore,' storeTypePage:',storeTypePage);
  console.log('to:',to, ' from:',from)

  if(refreshIndex >= 10){
    console.log('refresh limit');
    return next();
  }
  if(currentPage === storePage && storePage !=='/' && storePage !== START_ROUTE.path){
    console.log('Cyclic page refresh detected');
    return next();
  }
  if (authenticated && currentTypePage === 'popup') {
    console.log('skip steps');
    return next();
  }
  if(currentTypePage!=='popup'){ await localStorage?.setItem(CURRENT_PAGE, to.path);}
  if(storePage !== '/' &&
    storePage !== START_ROUTE.path &&
    storePage !== currentPage &&
    storeRestore &&
    !from?.name &&
    currentTypePage !== 'popup'){
     if(!storeRequiresAuth){
       console.log('step 0#1');
       return next({ path: storePage });
     }else{
        if (!authenticated) {
         console.log('step 0#2');
         return next({ path: START_ROUTE.path });
       }else{
         console.log('step 0#3');
         return next({ path: storePage });
       }
     }
   }

  if (!authenticated) {
    if(currentRequiresAuth){
      console.log('step 1#1');
      return next({ path: START_ROUTE.path });
    }else{
      console.log('step 1#2');
      return next();
    }
  }
  console.log('step 4');
  return next();
})

export default router
