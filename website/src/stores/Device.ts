import { Accessor, createMemo, createSignal, onCleanup, onMount } from "solid-js";
import { sendMessage, IWsMessage, websocket } from "../hooks/useWebSocket";

function readDeviceConfig(ws: WebSocket, deviceId: string): Promise<any> {
  return new Promise((resolve, reject) => {
    const handler = (event: MessageEvent) => {
      try {
        const data = JSON.parse(event.data);
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
    sendMessage({ type: "device-read-config", deviceId } as IWsMessage);
  });
}

function saveDeviceConfig(ws: WebSocket, deviceId: string, config: any): Promise<boolean> {
  return new Promise((resolve, reject) => {
    const handler = (event: MessageEvent) => {
      try {
        const data = JSON.parse(event.data);
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
    sendMessage({ type: "device-save-config", deviceId, config } as IWsMessage);
  });
}

export function createDeviceStore<TDeviceType extends keyof IDeviceStates>(
  deviceId: string,
  _deviceType: TDeviceType
) {
  // Device state
  const [state, setState] = createSignal<IDeviceStates[TDeviceType] | undefined>(undefined);
  const [config, setConfig] = createSignal<IDeviceConfigs[TDeviceType] | undefined>(undefined);
  const [error, setError] = createSignal<string | undefined>(undefined);
  const [connectionState, setConnectionState] = createSignal<number>(WebSocket.CONNECTING);

  // Load device state from backend
  const loadState = () => {
    sendMessage({ type: "device-get-state", deviceId } as IWsMessage);
  };

  function getChildStateByType<TDeviceType extends keyof IDeviceStates>(
    deviceType: TDeviceType
  ): Accessor<IDeviceStates[TDeviceType] | undefined> {
    const findChild: (state: IDeviceState | undefined) => IDeviceState | undefined = (state) => {
      for (const child of state?.children || []) {
        if (child.type === deviceType) {
          return child;
        }
        const result = findChild(child);
        if (result) return result;
      }
    };
    return createMemo(() => findChild(state()));
  }

  // Listen for state updates from WebSocket
  function onMessage(event: MessageEvent) {
    const data = JSON.parse(event.data);
    if (data.type === "device-state" && data.deviceId === deviceId) {
      if (data.error) {
        setError(data.error);
        setState(undefined);
      } else {
        setState(data.state);
        setError(undefined);
      }
      setConnectionState(WebSocket.OPEN);
    } else if (data.type === "device-read-config" && data.deviceId === deviceId) {
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
  }

  function handleClose() {
    setConnectionState(WebSocket.CLOSED);
  }

  onMount(() => {
    setConnectionState(WebSocket.CONNECTING);
    loadState();

    websocket.addEventListener("message", onMessage);
    websocket.addEventListener("open", handleOpen);
    websocket.addEventListener("close", handleClose);
  });
  onCleanup(() => {
    websocket.removeEventListener("close", handleClose);
    websocket.removeEventListener("open", handleOpen);
    websocket.removeEventListener("message", onMessage);
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
    loadState,
    loadConfig,
    saveConfig,
  };
}

export interface IDeviceState {
  id: string;
  name: string;
  type: string;
  children?: IDeviceState[];
}

export type IDeviceConfig = object;

declare global {
  export interface IDeviceStates {
    [key: string]: IDeviceState;
  }

  export interface IDeviceConfigs {
    [key: string]: IDeviceConfig;
  }
}
