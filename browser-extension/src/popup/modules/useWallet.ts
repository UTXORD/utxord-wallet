import { useStore } from '~/popup/store/index'
import { sendMessage } from 'webext-bridge'
import { GET_BALANCE, GET_ADDRESSES, GET_NETWORK } from '~/config/events'

const useWallet = () => {
  const store = useStore()

async function getNetWork(){
  const network = await sendMessage(GET_NETWORK, {}, 'background')
  console.log('network:',network)
  return network;
}
  async function getFundAddress() {
    const list = await sendMessage(GET_ADDRESSES, {}, 'background')
    const addresses = {};
    for(const item of list){
      addresses[item.type] = item;
    }
    if (addresses?.fund) {
      store.setFundAddress(addresses?.fund?.address)
      return addresses?.fund?.address
    }
  }
  async function getOrdAddress() {
    const list = await sendMessage(GET_ADDRESSES, {}, 'background')
    const addresses = {};
    for(const item of list){
      addresses[item.type] = item;
    }
    if (addresses?.ord) {
      store.setOrdAddress(addresses?.ord?.address)
      return addresses?.ord?.address
    }
  }

  async function getBalance(address: string) {
    if (address) {
      try {
        const success = await sendMessage(GET_BALANCE, {
          address,
        }, 'background')
        if (success) {
          store.setBalance(success?.data)
          console.log(success?.data);
        }
      } catch(e){
        store.setBalance(0);
        console.log('getBalance->error:',e);
      }
    };
  }

  async function saveDataForSign(data: {}) {
    store.setDataForSign(data || {})
  }

  async function saveDataForExportKeyPair(data: {}) {
    store.setDataForExportKeyPair(data || {})
  }

  return {
    getFundAddress,
    getOrdAddress,
    getBalance,
    saveDataForSign,
    saveDataForExportKeyPair,
    getNetWork,
  }
}

export default useWallet;
