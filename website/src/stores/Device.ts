import { Accessor, createSignal, onCleanup, onMount } from "solid-js";
import { sendMessage, websocket } from "../hooks/useWebSocket";
import { IWsReceiveMessage } from "../interfaces/WebSockets";

function readDeviceConfig(ws: WebSocket, deviceId: string): Promise<any> {
  return new Promise((resolve, reject) => {
    const handler = (event: MessageEvent) => {
      try {
        const messages = JSON.parse(event.data) as IWsReceiveMessage;
        messages.forEach((data) => {
          if (data.type === "device-read-config" && data.deviceId === deviceId) {
            ws.removeEventListener("message", handler);
            if ("error" in data) {
              reject(data.error);
            } else {
              resolve(data.config);
            }
          }
        });
      } catch {
        // Ignore non-JSON
      }
    };
    ws.addEventListener("message", handler);
    sendMessage({ type: "device-read-config", deviceId });
  });
}

function saveDeviceConfig(ws: WebSocket, deviceId: string, config: any): Promise<boolean> {
  return new Promise((resolve, reject) => {
    const handler = (event: MessageEvent) => {
      try {
        const messages = JSON.parse(event.data) as IWsReceiveMessage;
        messages.forEach((data) => {
          if (data.type === "device-save-config" && data.deviceId === deviceId) {
            ws.removeEventListener("message", handler);
            if ("error" in data) {
              reject(data.error);
            } else {
              resolve(true);
            }
          }
        });
      } catch {
        // Ignore non-JSON
      }
    };
    ws.addEventListener("message", handler);
    sendMessage({ type: "device-save-config", deviceId, config });
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
    sendMessage({ type: "device-state", deviceId });
  };

  function getChildStateByType<TDeviceType extends keyof IDeviceStates>(
    _deviceType: TDeviceType
  ): Accessor<IDeviceStates[TDeviceType] | undefined> {
    // TODO: Implement child device lookup
    return () => undefined;
  }

  // Listen for state updates from WebSocket
  function onMessage(event: MessageEvent) {
    try {
      const messages = JSON.parse(event.data) as IWsReceiveMessage;
      messages.forEach((data) => {
        if (data.type === "device-state" && data.deviceId === deviceId) {
          if ("error" in data) {
            setError(data.error);
            setState(undefined);
          } else if ("state" in data) {
            setState(data.state as any);
            setError(undefined);
          }
          setConnectionState(WebSocket.OPEN);
        } else if (data.type === "device-read-config" && data.deviceId === deviceId) {
          if ("error" in data) {
            setError(data.error);
            setConfig(undefined);
          } else if ("config" in data) {
            setConfig(data.config as any);
            setError(undefined);
          }
        }
      });
    } catch {
      // Ignore non-JSON
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
  type?: string;
  childrend?: IDeviceState[];
}

export type IDeviceConfig = object;

declare global {
  // These interfaces are augmented by device-specific files
  // eslint-disable-next-line @typescript-eslint/no-empty-object-type
  export interface IDeviceStates {}

  // eslint-disable-next-line @typescript-eslint/no-empty-object-type
  export interface IDeviceConfigs extends Record<string, any> {}
}
