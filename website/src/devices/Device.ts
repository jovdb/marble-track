import { createSignal, onCleanup, onMount } from "solid-js";
import { sendMessage, IWsMessage } from "../hooks/useWebSocket";

function readDeviceConfig(deviceId: string): Promise<any> {
  return new Promise((resolve, reject) => {
    const handler = (event: MessageEvent) => {
      try {
        const data = JSON.parse(event.data);
        if (data.type === "device-read-config" && data.deviceId === deviceId) {
          window.removeEventListener("message", handler);
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
    window.addEventListener("message", handler);
    sendMessage({ type: "device-read-config", deviceId } as IWsMessage);
  });
}

function saveDeviceConfig(deviceId: string, config: any): Promise<boolean> {
  return new Promise((resolve, reject) => {
    const handler = (event: MessageEvent) => {
      try {
        const data = JSON.parse(event.data);
        if (data.type === "device-save-config" && data.deviceId === deviceId) {
          window.removeEventListener("message", handler);
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
    window.addEventListener("message", handler);
    sendMessage({ type: "device-save-config", deviceId, config } as IWsMessage);
  });
}

export function createDeviceStore(deviceId: string) {
  // Device state
  const [state, setState] = createSignal<any>(undefined);
  const [config, setConfig] = createSignal<any>(undefined);
  const [error, setError] = createSignal<string | undefined>(undefined);

  // Load device state from backend
  const loadState = () => {
    sendMessage({ type: "device-get-state", deviceId } as IWsMessage);
  };

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
    }
  }

  onMount(() => {
    window.addEventListener("message", onMessage);
    loadState();
  });
  onCleanup(() => {
    window.removeEventListener("message", onMessage);
  });

  // Load config from backend
  const loadConfig = async () => {
    try {
      const cfg = await readDeviceConfig(deviceId);
      setConfig(cfg);
      setError(undefined);
    } catch (err: any) {
      setError(err?.toString() || "Failed to load config");
      setConfig(undefined);
    }
  };

  // Save config to backend
  const saveConfig = async (cfg: any) => {
    try {
      await saveDeviceConfig(deviceId, cfg);
      setConfig(cfg);
      setError(undefined);
    } catch (err: any) {
      setError(err?.toString() || "Failed to save config");
    }
  };

  return {
    state,
    config,
    error,
    loadState,
    loadConfig,
    saveConfig,
  };
}
