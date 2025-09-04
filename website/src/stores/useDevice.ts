import { createStore } from "solid-js/store";

export interface IDeviceStore<TState, TConfig> {
  deviceId: string;
  deviceType: string;
  name: string;
  state: TState;
  config: TConfig;
}

export interface IDeviceActions<TState, TConfig> {
  setName(name: string): void;
  setState(state: TState): void;
  setConfig(config: TConfig): void;
}

const dynamicStores: Record<string, [IDeviceStore<any, any>, IDeviceActions<any, any>]> = {};

export function useDevice<TState, TConfig>(deviceId: string) {
  // reuse store
  // If an instance changes a value all must update
  const result = dynamicStores[deviceId];
  if (result) {
    return result;
  }

  const [store, setStore] = createStore({
    deviceId,
    deviceType: "unknown",
    name: `Device ${deviceId}`,
    state: {} as TState,
    config: {} as TConfig,
  });

  const actions = {
    setName: (name: string) => setStore("name", name),
    setState: (state: TState) => setStore("state", state),
    setConfig: (config: TConfig) => setStore("config", config),
  };

  dynamicStores[deviceId] = [store, actions];
  return [store, actions] as const;
}
