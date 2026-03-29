import { createEffect, createContext, onCleanup, onMount, useContext } from "solid-js";
import { createStore } from "solid-js/store";
import { IWebSocketActions, useWebSocket2 } from "../hooks/useWebSocket";
import { SystemInfo } from "../interfaces/WebSockets";

export interface ISystemInfoStore extends SystemInfo {
  error: string | null;
  lastUpdatedAt: number | null;
}

export function createSystemInfoStore({ subscribe }: Pick<IWebSocketActions, "subscribe">) {
  const [store, setStore] = createStore<ISystemInfoStore>(
    {
      serialBaudRate: 115200,
      firmwareBuild: "",
      hostname: "",
      ipAddress: "",
      connectionInfo: "",
      freeHeap: undefined,
      uptimeMs: undefined,
      webSocketClients: undefined,
      resetReason: "",
      chipModel: "",
      sdkVersion: "",
      error: null,
      lastUpdatedAt: null,
    },
    { name: "systemInfoStore" }
  );

  const handleMessage: Parameters<IWebSocketActions["subscribe"]>[0] = (message) => {
    if (message.type !== "system-info") {
      return;
    }

    if ("error" in message) {
      setStore({ error: message.error, lastUpdatedAt: Date.now() });
      return;
    }

    setStore({
      serialBaudRate: message.serialBaudRate,
      firmwareBuild: message.firmwareBuild,
      hostname: message.hostname ?? "",
      ipAddress: message.ipAddress ?? "",
      connectionInfo: message.connectionInfo ?? "",
      freeHeap: message.freeHeap,
      uptimeMs: message.uptimeMs,
      webSocketClients: message.webSocketClients,
      resetReason: message.resetReason ?? "",
      chipModel: message.chipModel ?? "",
      sdkVersion: message.sdkVersion ?? "",
      error: null,
      lastUpdatedAt: Date.now(),
    });
  };

  onMount(() => {
    const cleanup = subscribe(handleMessage);

    onCleanup(() => {
      cleanup();
    });
  });

  return store;
}

const SystemInfoContext = createContext<ISystemInfoStore>({
  serialBaudRate: 115200,
  firmwareBuild: "",
  hostname: "",
  ipAddress: "",
  connectionInfo: "",
  freeHeap: undefined,
  uptimeMs: undefined,
  webSocketClients: undefined,
  resetReason: "",
  chipModel: "",
  sdkVersion: "",
  error: null,
  lastUpdatedAt: null,
});

export function SystemInfoProvider(props: { children: any }) {
  const [webSocket, actions] = useWebSocket2();
  const store = createSystemInfoStore(actions);

  createEffect(() => {
    if (webSocket.isConnected) {
      actions.sendMessage({ type: "system-info" });
    }
  });

  return <SystemInfoContext.Provider value={store}>{props.children}</SystemInfoContext.Provider>;
}

export function useSystemInfo() {
  return [useContext(SystemInfoContext)] as const;
}
