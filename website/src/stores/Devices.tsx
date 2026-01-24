import { createStore, produce } from "solid-js/store";
import { createContext, onCleanup, onMount, useContext } from "solid-js";
import { IWebSocketActions, useWebSocket2 } from "../hooks/useWebSocket";
import { DeviceInfo } from "../interfaces/WebSockets";

export type IDeviceConfig = Record<string, unknown>;

export type IDeviceState = Record<string, unknown>;

export interface IDevice<
  TState extends IDeviceState = IDeviceState,
  TConfig extends IDeviceConfig = IDeviceConfig,
> {
  id: string;
  type: string;
  pins?: number[];
  /** Generic features mirrored from firmware mixins */
  features?: string[];
  state?: TState;
  stateErrorMessage?: string;
  config?: TConfig;
  configErrorMessage?: string;
  children?: {
    id: string;
    type: string;
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
              const oldDevices = draft.devices;

              // Build a new devices object in the order provided by the message
              // This ensures reordering is reflected in the UI
              const newDevices: IDevices = {};

              // Helper function to recursively add devices in order
              const addDeviceRecursively = (device: DeviceInfo) => {
                const existingDevice = oldDevices[device.id];
                newDevices[device.id] = {
                  id: device.id,
                  type: device.type,
                  pins: device.pins,
                  features: device.features,
                  // Preserve existing state and config if available
                  state: existingDevice?.state,
                  stateErrorMessage: existingDevice?.stateErrorMessage,
                  config: existingDevice?.config,
                  configErrorMessage: existingDevice?.configErrorMessage,
                  children:
                    device.children?.map((child) => ({ id: child.id, type: child.type })) || [],
                };

                // Recursively process children
                device.children?.forEach(addDeviceRecursively);
              };

              // Add/update all devices recursively in order
              devices.forEach(addDeviceRecursively);

              // Replace the devices object entirely to preserve order
              draft.devices = newDevices;
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
      case "reorder-devices": {
        if ("error" in message) {
          console.error("Failed to reorder devices:", message.error);
          alert(`Failed to reorder devices: ${message.error}`);
        } else if ("success" in message) {
          console.log("Devices reordered successfully");
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
        } else if ("config" in message) {
          setStore(
            produce((draft) => {
              const draftDevice = draft.devices[message.deviceId];
              if (draftDevice) {
                draftDevice.config = message.config;
                draftDevice.configErrorMessage = undefined;
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
      case "device-save-config": {
        if ("success" in message && !message.success) {
          setStore(
            produce((draft) => {
              const draftDevice = draft.devices[message.deviceId];
              if (draftDevice) {
                draftDevice.configErrorMessage =
                  (message as any).message || (message as any).error || "Unknown error";
              }
            })
          );
        } else if ("config" in message) {
          setStore(
            produce((draft) => {
              const draftDevice = draft.devices[message.deviceId];
              if (draftDevice) {
                draftDevice.config = message.config ?? {};
                draftDevice.configErrorMessage = undefined;
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

  const getDeviceConfig = (deviceId: string) =>
    sendMessage({ type: "device-read-config", deviceId });
  const setDeviceConfig = (deviceId: string, config: IDeviceConfig) =>
    sendMessage({ type: "device-save-config", deviceId, config });
  const getDeviceState = (deviceId: string) => sendMessage({ type: "device-state", deviceId });

  return [
    /** Don't destructure! */
    store,
    {
      loadDevices,
      getDeviceConfig,
      setDeviceConfig,
      getDeviceState,
      sendMessage,
    },
  ] as const;
}

export function useDevice<TState extends IDeviceState, TConfig extends IDeviceConfig>(
  deviceId: string
) {
  const store = useContext(DevicesContext);

  const [, { getDeviceConfig, setDeviceConfig, getDeviceState, sendMessage }] = useDevices();

  // tracking only needed once
  onMount(() => {
    getDeviceConfig(deviceId);
    getDeviceState(deviceId);
  });

  return [
    store.devices[deviceId] as IDevice<TState, TConfig> | undefined,
    {
      getDeviceConfig: () => getDeviceConfig(deviceId),
      getDeviceState: () => getDeviceState(deviceId),
      setDeviceConfig: (config: TConfig) => setDeviceConfig(deviceId, config),
      sendMessage,
    },
  ] as const;
}
