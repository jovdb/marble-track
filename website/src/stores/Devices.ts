/*
import { Accessor, createMemo, createSignal, onCleanup, onMount } from "solid-js";
import { sendMessage, IWsMessageBase, websocket } from "../hooks/useWebSocket";
import { createStore } from "solid-js/store";

function readDeviceConfig(ws: WebSocket, deviceId: string): Promise<any> {
  return new Promise((resolve, reject) => {
    const handler = (event: MessageEvent) => {
      try {
        const data = JSON.parse(event.data) as IWsReceiveMessage;
        if (data.type === "device-read-config" && data.deviceId === deviceId) {
          ws.removeEventListener("message", handler);
          if (data.error) {
            reject(data.error);
          } else {
            resolve(data.config);
          }
        }
      } catch {
        // Ignore non-JSON
      }
    };
    ws.addEventListener("message", handler);
    sendMessage({ type: "device-read-config", deviceId } as IWsMessageBase);
  });
}

function saveDeviceConfig(ws: WebSocket, deviceId: string, config: any): Promise<boolean> {
  return new Promise((resolve, reject) => {
    const handler = (event: MessageEvent) => {
      try {
        const data = JSON.parse(event.data) as IWsReceiveMessage;
        if (data.type === "device-save-config" && data.deviceId === deviceId) {
          ws.removeEventListener("message", handler);
          if (data.error) {
            reject(data.error);
          } else {
            resolve(true);
          }
        }
      } catch {
        // Ignore non-JSON
      }
    };
    ws.addEventListener("message", handler);
    sendMessage({ type: "device-save-config", deviceId, config } as IWsMessageBase);
  });
}

export function createDevicesStore<IDevices>()
   
  // Device state
  const [state, setState] = createSignal<IDeviceStates[TDeviceType] | undefined>(undefined);
  const [config, setConfig] = createSignal<IDeviceConfigs[TDeviceType] | undefined>(undefined);
  const [error, setError] = createSignal<string | undefined>(undefined);
  const [connectionState, setConnectionState] = createSignal<number>(WebSocket.CONNECTING);

  const store = createStore<IDevices>({

  });

  // Load devices
  const loadDevices = () => {
   sendMessage({ type: "get-devices" });
  };


  // Listen for state updates from WebSocket
  function onMessage(event: MessageEvent) {
    const data = JSON.parse(event.data) as IWsReceiveMessage;

    if (data.type === "devices") {
      if (data.error) {
        setError(data.error);
        setState(undefined);
      } else {
        setState(data.state);
        setError(undefined);
      }
    } else if (data.type === "device-state") {
      if (data.error) {
        setError(data.error);
        setState(undefined);
      } else {
        setState(data.state);
        setError(undefined);
      }
      setConnectionState(WebSocket.OPEN);
    
    } else if (data.type === "device-config") {
      if (data.error) {
        setError(data.error);
        setConfig(undefined);
      } else {
        setConfig(data.config);
        setError(undefined);
      }
    }
  }

  function handleOpen() {
    setConnectionState(WebSocket.OPEN);

    // Load devices
    sendMessage({ type: "get-devices" });
  }

  function handleClose() {
    setConnectionState(WebSocket.CLOSED);
  }

  onMount(() => {
    setConnectionState(WebSocket.CONNECTING);

    websocket.addEventListener("open", handleOpen);
    websocket.addEventListener("close", handleClose);
    websocket.addEventListener("message", onMessage);
  });

  onCleanup(() => {
    websocket.removeEventListener("message", onMessage);
    websocket.removeEventListener("close", handleClose);
    websocket.removeEventListener("open", handleOpen);
  });

  // Load config from backend
  const loadConfig = () => {
    readDeviceConfig(websocket, deviceId);
  };

  // Save config to backend
  const saveConfig = (cfg: any) => {
    saveDeviceConfig(websocket, deviceId, cfg);
  };

  return {
    state,
    getChildStateByType,
    config,
    error,
    connectionState,
    loadState: loadDevices,
    loadConfig,
    saveConfig,
  };
}

export type IDeviceState = object;

export interface IDevice {
  id: string;
  type: string;
  state: IDeviceState;
  config: IDeviceConfig;
  children?: IDevice[];
}

export type IDevices = Record<string, IDevice>;

export type IDeviceConfig = object;

declare global {
  export interface IDeviceStates {
    [key: string]: IDeviceState;
  }

  export interface IDeviceConfigs {
    [key: string]: IDeviceConfig;
  }
}
*/
