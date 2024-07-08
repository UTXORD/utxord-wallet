import { BACKEND_URL } from '~/config/index'
import * as Sentry from "@sentry/browser"
class Rest {
  constructor() {}

  async get(path: string, headers = {}) {
    try{
      if(!path) return;
      const fullPath = `${BACKEND_URL}${path}`;
      if(fetch){
        const response = await fetch(fullPath, {
          method: 'GET',
          headers,
        });
        if(response?.ok){
          const json = await response?.json();
          return json;
        }
        console.log('error connect:', response.status);
        return;
      }
    }catch(e){
      console.log(`Rest::get->error: ${e.message}`, e.stack);
    }
  }

  async getTimeServer(headers = {}) {
    try{
      const fullPath = 'https://worldtimeapi.org/api/timezone/Etc/UTC.json';
      //'http://worldclockapi.com/api/json/utc/now';
      if(fetch){
        const response = await fetch(fullPath, {
          method: 'GET',
          headers,
          });
        if(response?.ok){
          const json = await response?.json();
          return json?.datetime;
        }
        console.log('error connect:', response.status);
        return;
      }
    }catch(e){
      console.log(`Rest::get->error: ${e.message}`, e.stack);
      return;
    }
  }

}

export default Rest;
