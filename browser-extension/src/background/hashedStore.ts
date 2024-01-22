import {Md5} from 'ts-md5';
import json5 from 'json5';


export class HashedStore {
  private static _instance: HashedStore;
  private static readonly NAME_PREFIX: string = "utxord_wallet.hashed_store";
  private static readonly DEFAULT_NAME: string = "default";
  private readonly _name: string;
  private readonly _items: {[index: string]: any};

  private constructor(name: string) {
    this._name = `${HashedStore.NAME_PREFIX}.${name}`;
    this._items = {};
  }

  public static getInstance(): HashedStore {
    if (!HashedStore._instance) {
      HashedStore._log("create instance");
      HashedStore._instance = new HashedStore(HashedStore.DEFAULT_NAME);
    }
    return HashedStore._instance;
  }

  private static _log(msg: string) {
      console.debug(`===== HashedStore: ${msg}`);
  }

  public clear(key: string) {
    delete this._items[key];
  }

  public put(item: any, key: string | null = null): string {
    let itemValue = {value: item};
    const itemKey = key || Md5.hashStr(json5.stringify(itemValue));
    this._items[itemKey] = itemValue;
    return itemKey;
  }

  public get(key: string): any {
    return this._items[key]?.value;
  }

  public pop(key: string): any {
    let value = this.get(key);
    this.clear(key);
    return value;
  }
}
