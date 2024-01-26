import { BACKEND_URL } from '~/config/index'

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
        if(response){
          const json = await response?.json();
          return json;
        }
      }
    }catch(e){
      console.error(`Rest::get->error: ${e.message}`, e.stack);
    }
  }
}

export default Rest;
