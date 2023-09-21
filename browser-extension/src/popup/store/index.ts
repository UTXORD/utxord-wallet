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
  fundAddress: string | null
  ordAddress: string | null
  dataForSign: {} | null
  dataForExportKeyPair: {} | null
  errorMessage: string | null
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
    fundAddress: null,
    ordAddress: null,
    dataForSign: null,
    dataForExportKeyPair: null,
    errorMessage: null,
  } as IStore),
  actions: {
    setBalance(value: IBalance) {
      this.balance = value
    },
    setFundAddress(addr: string) {
      this.fundAddress = addr
    },
    setOrdAddress(addr: string) {
      this.ordAddress = addr
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
    clearStore() {
      this.balance = {
        confirmed: 0,
        unconfirmed: 0,
        inscriptions: [],
        connect: false,
        sync: false,
        to_address: 0,
        used_for_inscribtions: 0,
      }
      this.fundAddress = null
      this.ordAddress = null
      this.dataForSign = null
      this.dataForExportKeyPair = null
      this.errorMessage = null
    }
  },
  persist: true,
})
