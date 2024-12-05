import * as webext from "webext-bridge";

const sendMessageLimit = 5
const sendMessageTimeout = 100

export async function sendMessage(type, message, destination, index?: number){
  let output = null;
  if(!index){
    index = 0;
  }
  try {
    output = await webext.sendMessage(type, message, destination)
  } catch (error) {
    console.log('sendMessageError:', error)
    if(index >=  sendMessageLimit){
       console.warn('sendMessageError: limit has expired');
       return output;
     }
    return setTimeout(() => {
      index += 1
      return sendMessage(type, message, destination, index);
    }, sendMessageTimeout);
  }
  return output;
}
