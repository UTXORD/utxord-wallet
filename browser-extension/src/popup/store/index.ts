import { defineStore } from 'pinia'

interface IBalance {
  confirmed: number;
  unconfirmed: number;
  connect: boolean;
  inscriptions: any[];
  sync: boolean;
  to_address: number;
  used_for_inscribtions: number;
}

interface IStore {
  balance: IBalance;
  addresses: any[] | null
  fundAddress: string | null
  ordAddress: string | null
  typeAddress: number
  dataForSign: {} | null
  dataForExportKeyPair: {} | null
  errorMessage: string | null
  usdRate: number
  useDerivation: boolean
  errorReporting: boolean
  viewMode: boolean
}

export const useStore = defineStore('store', {
  state: () => ({
    balance: {
      confirmed: 0,
      unconfirmed: 0,
      inscriptions: [],
      connect: false,
      sync: false,
      to_address: 0,
      used_for_inscribtions: 0,
    },
    refreshingBalance: false,
    useDerivation: false,
    errorReporting: false,
    viewMode: false,
    fundAddress: null,
    ordAddress: null,
    addresses: null,
    typeAddress: 0,
    dataForSign: null,
    dataForExportKeyPair: null,
    errorMessage: null,
    usdRate: 0,
  } as IStore),
  getters: {
    getUSDRate: (state) => state.usdRate || 0,
    getAddresses: (state) => state.addresses || [],
  },
  actions: {
    setUSD(value: number) {
      this.usdRate = value
    },
    setBalance(value: IBalance) {
      // console.log('... store.setBalance: ', value);
      this.balance = value
    },
    setFundAddress(addr: string) {
      this.fundAddress = addr
    },
    setOrdAddress(addr: string) {
      this.ordAddress = addr
    },
    setAddresses(list: any[]) {
      this.addresses = list
    },
    setUseDerivation(val: boolean){
      this.useDerivation = Boolean(val)
    },
    setErrorReporting(val: boolean){
      this.errorReporting = Boolean(val)
    },
    setViewMode(val: boolean){
      this.viewMode = Boolean(val)
    },
    setTypeAddress(type: number){
      this.typeAddress = Number(type)
    },
    setDataForSign(data: {}) {
      this.dataForSign = data
    },
    setDataForExportKeyPair(data: {}) {
      this.dataForExportKeyPair = data
    },
    setSyncToFalse() {
      this.balance.sync = false;
    },
    setRefreshingBalance() {
      this.refreshingBalance = true;
    },
    unsetRefreshingBalance() {
      this.refreshingBalance = false;
    },
    setErrorMessage(msg: string) {
      this.errorMessage = msg
    },
    clearBalance() {
      this.balance = {
        confirmed: 0,
        unconfirmed: 0,
        inscriptions: [],
        connect: false,
        sync: false,
        to_address: 0,
        used_for_inscribtions: 0,
      }
    },
    clearStore() {
      this.clearBalance()
      this.fundAddress = null
      this.addresses = null
      this.ordAddress = null
      this.dataForSign = null
      this.dataForExportKeyPair = null
      this.errorMessage = null
      this.useDerivation = false
      this.errorReporting = false
      this.viewMode = false
    }
  },
  persist: true,
})
