import { createStore, produce } from "solid-js/store";
import { createContext, onCleanup, onMount, useContext } from "solid-js";
import { IWebSocketActions, useWebSocket2 } from "../hooks/useWebSocket";

export type IDeviceConfig = Record<string, unknown>;

export type IDeviceState = Record<string, unknown>;

export interface IDevice<
  TState extends IDeviceState = IDeviceState,
  TConfig extends IDeviceConfig = IDeviceConfig,
> {
  id: string;
  type: string;
  pins?: number[];
  state?: TState;
  stateErrorMessage?: string;
  config?: TConfig;
  configErrorMessage?: string;
  children?: {
    id: string;
    type: string;
    pins?: number[];
  }[]; // Array of child device IDs
}

export type IDevices = Record<string, IDevice>;
export type IDevicesStore = { devices: IDevices };

export function createDevicesStore({
  sendMessage,
  subscribe,
}: Pick<IWebSocketActions, "subscribe" | "sendMessage">) {
  const [store, setStore] = createStore<IDevicesStore>({ devices: {} }, { name: "devicesStore" });

  // for debugging
  (window as any).__store = store;

  const handleMessage: Parameters<IWebSocketActions["subscribe"]>[0] = (message) => {
    switch (message.type) {
      case "devices-list": {
        if ("devices" in message) {
          setStore(
            produce((draft) => {
              const { devices } = message;
              // Remove unknown devices
              Object.keys(draft.devices).forEach((key) => {
                if (
                  !devices.find((d) => d.id === key) &&
                  !devices.some((d) => d.children?.find((c) => c.id === key))
                ) {
                  delete draft.devices[key];
                }
              });

              // Add update devices and their children
              devices.forEach((device) => {
                const deviceDraft = draft.devices[device.id];
                if (deviceDraft) {
                  // Update device
                  deviceDraft.type = device.type;
                  deviceDraft.pins = device.pins;
                  deviceDraft.children = device.children || [];
                } else {
                  // Add device
                  draft.devices[device.id] = {
                    id: device.id,
                    type: device.type,
                    pins: device.pins,
                    state: undefined,
                    config: undefined,
                    children: device.children || [],
                  };
                }

                // Add child devices to the store
                device.children?.forEach((child) => {
                  const childDraft = draft.devices[child.id];
                  if (childDraft) {
                    // Update child device
                    childDraft.type = child.type;
                    childDraft.pins = child.pins;
                    childDraft.children = child.children || [];
                  } else {
                    // Add child device
                    draft.devices[child.id] = {
                      id: child.id,
                      type: child.type,
                      pins: child.pins,
                      state: undefined,
                      config: undefined,
                      children: child.children || [],
                    };
                  }
                });
              });
            })
          );
        }
        break;
      }
      case "add-device": {
        if ("error" in message) {
          console.error("Failed to add device:", message.error);
          alert(`Failed to add device: ${message.error}`);
        } else if ("success" in message) {
          console.log("Device added successfully:", message.deviceId);
          // Device list will be refreshed automatically via devices-list message
        }
        break;
      }
      case "remove-device": {
        if ("error" in message) {
          console.error("Failed to remove device:", message.error);
          alert(`Failed to remove device: ${message.error}`);
        } else if ("success" in message) {
          console.log("Device removed successfully:", message.deviceId);
          // Device list will be refreshed automatically via devices-list message
        }
        break;
      }
      case "device-config": {
        if ("config" in message) {
          setStore(
            produce((draft) => {
              const draftDevice = draft.devices[message.deviceId];
              if (draftDevice) {
                draftDevice.config = message.config ?? {};
                draftDevice.configErrorMessage = undefined;
              }
            })
          );

          // Refresh devices list to get updated pins
          if (message.triggerBy === "set" || message.isChanged) {
            sendMessage({ type: "devices-list" });
          }
        } else if ("success" in message && !message.success) {
          setStore(
            produce((draft) => {
              const draftDevice = draft.devices[message.deviceId];
              if (draftDevice) {
                draftDevice.config = undefined;
                draftDevice.configErrorMessage =
                  (message as any).message || (message as any).error || "Unknown error";
              }
            })
          );
        }
        break;
      }
      case "device-read-config": {
        if ("success" in message && !message.success) {
          setStore(
            produce((draft) => {
              const draftDevice = draft.devices[message.deviceId];
              if (draftDevice) {
                draftDevice.config = undefined;
                draftDevice.configErrorMessage =
                  (message as any).message || (message as any).error || "Unknown error";
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
                const { id, type, ...rest } = message.state;
                draftDevice.state = rest;
                draftDevice.stateErrorMessage = undefined;
              }
            })
          );
        } else if ("error" in message || ("success" in message && !message.success)) {
          setStore(
            produce((draft) => {
              const draftDevice = draft.devices[message.deviceId];
              if (draftDevice) {
                draftDevice.stateErrorMessage =
                  (message as any).message || (message as any).error || "Unknown error";
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
      type: "devices-list",
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
  const loadDevices = () => sendMessage({ type: "devices-list" });

  return [
    /** Don't destructure! */
    store,
    {
      loadDevices,
    },
  ] as const;
}

export function useDevice<TState extends IDeviceState, TConfig extends IDeviceConfig>(
  deviceId: string
) {
  const store = useContext(DevicesContext);
  const [, { sendMessage }] = useWebSocket2();

  const getDeviceConfig = () => sendMessage({ type: "device-read-config", deviceId });
  const setDeviceConfig = (config: TConfig) =>
    sendMessage({ type: "device-save-config", deviceId, config });
  const getDeviceState = () => sendMessage({ type: "device-state", deviceId });

  // tracking only needed once
  onMount(() => {
    getDeviceConfig();
    getDeviceState();
  });

  return [
    store.devices[deviceId] as IDevice<TState, TConfig> | undefined,
    { getDeviceConfig, getDeviceState, setDeviceConfig, sendMessage },
  ] as const;
}
