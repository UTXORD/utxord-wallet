import winHelpers from '~/helpers/winHelpers';
import {sendMessage} from 'webext-bridge';
import {
  EXCEPTION,
  WARNING,
  NOTIFICATION
  } from '~/config/events';
export default class Errors{

    async handlerForWasmWrapper(obj, name = '', props = {}, list = [], args = 0, proto = false, lvl = 0){
      const out = {name, props, list, args, proto, lvl};
      const myself = this;
      if(!obj) return out;
      let methods = Object.getOwnPropertyNames(obj).filter((n) => n[0]!== '_');
      if(methods.length === 3 && methods.indexOf('length') !== -1 && methods.indexOf('prototype') !== -1){
        out.args = obj?.length || 0;
        out.lvl += 1;
        out.proto = true;
        return await this.handlerForWasmWrapper(
          obj.prototype,
          out.name,
          out.props,
          out.list,
          out.args,
          out.proto,
          out.lvl
        );
      }
      for (let m of methods){
        if((typeof obj[m]) === 'function' &&
          m.indexOf('dynCall') === -1 &&
          m.indexOf('constructor') === -1 &&
          out.lvl < 8){
          out.list.push(m);
          let prps = await this.handlerForWasmWrapper(obj[m],m,{},[],0,false, out.lvl+1);
          out.props[m] = prps;
          obj[`$_${m}`] = obj[m];
          obj[`$_${m}`].prototype = obj[m].prototype;
          obj[`_${m}`] = function(){
              try{
                let o;
                if(!out.proto && out.lvl === 0){
                  o = new (obj[`$_${m}`])(...arguments);
                }else{
                  o = this[`$_${m}`](...arguments);
                }
                return o;
              }catch(e){
                myself.sendErrorMessage(m, e);
                return null;
              }
          };
          delete(obj[m]);
          obj[m] = obj[`_${m}`];
          obj[m].prototype = obj[`$_${m}`].prototype;
          delete(obj[`_${m}`]);

        }
      }
      return out;
    }

    async handler(obj, name = '', props = {}, list = [], args = 0, proto = false, lvl = 0){
      const out = {name, props, list, args, proto, lvl};
      const myself = this;
      if(!obj) return out;
      let methods = [...Object.getOwnPropertyNames(obj),...Object.getOwnPropertyNames(obj.__proto__)];//#.filter((n) => n[0]!== '_');
      //console.log(name,'|bf|->','methods:',methods)
      methods = methods.filter((n) =>
          n !== 'caller' &&
          n !== 'callee' &&
          n !== 'arguments' &&
          n !== 'apply' &&
          n !== 'bind' &&
          n !== 'call' &&
          n !== 'Object' &&
//          n !== 'prototype' &&
          n !== 'length' &&
//          n !== 'name' &&
//          n !== 'constructor' &&
          n !== 'toString'
        );
      //console.log(name,'|af|->','methods:',methods)
      if(methods.length === 3 && methods.indexOf('length') !== -1 && methods.indexOf('prototype') !== -1){
        out.args = obj?.length || 0;
        out.lvl += 1;
        out.proto = true;
        return await this.handler(
          obj.prototype,
          out.name,
          out.props,
          out.list,
          out.args,
          out.proto,
          out.lvl
        );
      }
      for (let m of methods){
        if((typeof obj[m]) === 'function' &&
          m.indexOf('dynCall') === -1 &&
          m.indexOf('constructor') === -1 &&
          out.lvl < 8){
          out.list.push(m);
          let prps = await this.handler(obj[m],m,{},[],0,false, out.lvl+1);
          out.props[m] = prps;
          obj[`$_${m}`] = obj[m];
          obj[`$_${m}`].prototype = obj[m].prototype;
          obj[`_${m}`] = function(){
              try{
                let o;
                 if(!out.proto && out.lvl === 0){
                   o = obj[`$_${m}`](...arguments);
                 }else{
                  o = this[`$_${m}`](...arguments);
                 }
                return o;
              }catch(e){
                console.log('m:',m,'|out.proto:',out.proto,'|out.lvl:',out.lvl,'|typeof obj[m]:',(typeof obj[`$_${m}`]))
                const json = JSON.stringify(obj);
                myself.sendErrorMessage(m, e);
                return null;
              }
          };
          delete(obj[m]);
          obj[m] = obj[`_${m}`];
          obj[m].prototype = obj[`$_${m}`].prototype;
          delete(obj[`_${m}`]);

        }
      }
      return out;
    }
    async sendErrorMessage(m,e){
      console.log('Error->','m:',m, 'e:',e);
      const wh = new winHelpers();
      const currentWindow = await wh.getCurrentWindow()
      sendMessage(EXCEPTION, `${m}->${e}`, `popup@${currentWindow.id}`)
    }

}
