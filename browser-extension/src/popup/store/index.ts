import { defineStore } from 'pinia'

interface IBalance {
  confirmed: number,
  unconfirmed: number,
}

interface IStore {
  balance: IBalance;
  fundAddress: string | null
  ordAddress: string | null
  dataForSign: {} | null
  dataForExportKeyPair: {} | null
}

export const useStore = defineStore('store', {
  state: () => ({
    balance: {
      confirmed: 0,
      unconfirmed: 0,
    },
    fundAddress: null,
    ordAddress: null,
    dataForSign: null,
    dataForExportKeyPair: null,
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
    clearStore() {
      this.balance = {
        confirmed: 0,
        unconfirmed: 0,
      }
      this.fundAddress = null
      this.ordAddress = null
      this.dataForSign = null
      this.dataForExportKeyPair = null
    }
  },
  persist: true,
})
