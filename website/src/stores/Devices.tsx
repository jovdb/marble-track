import { createStore, produce } from "solid-js/store";
import { createContext, onCleanup, onMount, useContext } from "solid-js";
import { IWebSocketActions, useWebSocket2 } from "../hooks/useWebSocket2";

export type IDeviceConfig = object;

export type IDeviceState = object;

export interface IDevice<
  TState extends IDeviceState = IDeviceState,
  TConfig extends IDeviceConfig = IDeviceConfig,
> {
  id: string;
  type: string;
  state?: TState;
  config?: TConfig;
  children?: IDevice[];
}

export type IDevices = Record<string, IDevice>;
export type IDevicesStore = { devices: IDevices };

declare global {
  export interface IDeviceStates {
    [key: string]: IDeviceState;
  }

  export interface IDeviceConfigs {
    [key: string]: IDeviceConfig;
  }
}

export function createDevicesStore({
  sendMessage,
  subscribe,
}: Pick<IWebSocketActions, "subscribe" | "sendMessage">) {
  const [store, setStore] = createStore<IDevicesStore>({ devices: {} }, { name: "devicesStore" });

  const handleMessage: Parameters<IWebSocketActions["subscribe"]>[0] = (message) => {
    switch (message.type) {
      case "devices-list": {
        if ("devices" in message) {
          setStore(
            produce((draft) => {
              const { devices } = message;
              // Remove unknown devices
              Object.keys(draft).forEach((key) => {
                if (!devices.find((d) => d.id === key)) {
                  delete draft.devices[key];
                }
              });

              // Add update devices
              devices.forEach((device) => {
                const deviceDraft = draft.devices[device.id];
                if (deviceDraft) {
                  // Update device
                  deviceDraft.type = device.type;
                } else {
                  // Add device
                  draft.devices[device.id] = {
                    id: device.id,
                    type: device.type,
                    state: undefined,
                    config: undefined,
                    // children: device.children || [],
                  };
                }
              });
            })
          );
        }
        break;
      }
      case "device-config": {
        if ("config" in message) {
          setStore(
            produce((draft) => {
              const draftDevice = draft.devices[message.deviceId];
              if (draftDevice) {
                draftDevice.config = message.config;
              }
            })
          );
        }
        break;
      }
      case "device-state": {
        if ("state" in message) {
          setStore(
            produce((draft) => {
              const draftDevice = draft.devices[message.deviceId];
              if (draftDevice) {
                draftDevice.state = message.state;
              }
            })
          );
        }
        break;
      }
    }
  };

  // Start getting a list of available devices
  onMount(() => {
    const cleanup = subscribe(handleMessage);

    sendMessage({
      type: "get-devices",
    });

    onCleanup(() => {
      cleanup();
    });
  });

  return store || {};
}

const DevicesContext = createContext<IDevicesStore>({ devices: {} });

export function DevicesProvider(props: { children: any }) {
  const [, actions] = useWebSocket2();
  const store = createDevicesStore(actions);
  return <DevicesContext.Provider value={store}>{props.children}</DevicesContext.Provider>;
}

export function useDevices() {
  const store = useContext(DevicesContext);
  const [, { sendMessage }] = useWebSocket2();
  const loadDevices = () => sendMessage({ type: "get-devices" });

  return [
    /** Don't destructure! */
    store,
    {
      loadDevices,
    },
  ] as const;
}

export function useDevice(deviceId: string) {
  const store = useContext(DevicesContext);
  const [, { sendMessage }] = useWebSocket2();

  const getDeviceConfig = () => sendMessage({ type: "device-read-config", deviceId });
  const getDeviceState = () => sendMessage({ type: "device-get-state", deviceId });

  // tracking only needed once
  onMount(() => {
    getDeviceConfig();
    getDeviceState();
  });

  return [store.devices[deviceId], { getDeviceConfig, getDeviceState, sendMessage }] as const;
}
