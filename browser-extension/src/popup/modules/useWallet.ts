import { computed } from 'vue'
import { useStore } from '~/popup/store/index'
import { sendMessage } from 'webext-bridge'
import { GET_BALANCE, GET_USD_RATE, GET_ADDRESSES, GET_NETWORK } from '~/config/events'

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
        // console.log(`===== getBalance(), address: ${address}`);
        const balance = await sendMessage(GET_BALANCE, {
          address,
        }, 'background')
        // console.log('===== getBalance(), balance:');
        // console.dir(balance);
        if (balance) {
          store.setBalance(balance?.data)
        }
      } catch(e) {
        console.log('getBalance->error:',e);
      }
    };
  }

  async function saveMetadataForSign(data: {}) {
    store.setMetadataForSign(data || {});
    store.setErrorMessage(data['errorMessage'] || null);
  }

  async function saveDataForExportKeyPair(data: {}) {
    store.setDataForExportKeyPair(data || {})
  }

  async function fetchUSDRate() {
    try {
      const usdRate = await sendMessage(GET_USD_RATE, {}, 'background')
      if (usdRate?.data?.USD) {
        store.setUSD(usdRate.data.USD || 0)
      }
    } catch (e) {
      console.log('fetchUSDRate->error:', e);
    }
  }

  const usdRate = computed((): number => store.getUSDRate || 0);

  return {
    getFundAddress,
    getOrdAddress,
    getBalance,
    saveMetadataForSign,
    saveDataForExportKeyPair,
    getNetWork,
    fetchUSDRate,
    usdRate
  }
}

export default useWallet;
