import { createSignal, onMount, onCleanup } from "solid-js";
import { sendMessage } from "../hooks/useWebSocket";

export interface DeviceInfo {
  id: string;
  name: string;
  type: string;
  state: any;
}

// Global device store
const [availableDevices, setAvailableDevices] = createSignal<DeviceInfo[]>([]);
const [devicesLoaded, setDevicesLoaded] = createSignal(false);
const [devicesLoading, setDevicesLoading] = createSignal(false);

// WebSocket message handler for device list
let messageHandler: ((event: MessageEvent) => void) | null = null;

export function useDeviceStore() {
  // Request devices from the server
  const requestDevices = () => {
    if (sendMessage(JSON.stringify({ type: "get-devices" }))) {
      setDevicesLoading(true);
      console.log("Requested device list from server");
    } else {
      console.error("Failed to request device list - WebSocket not connected");
    }
  };

  // Setup WebSocket listener
  onMount(() => {
    messageHandler = (event: MessageEvent) => {
      try {
        const data = JSON.parse(event.data);
        if (data.type === "devices-list") {
          console.log("Received devices list:", data);

          if (data.error) {
            console.error("Error getting devices:", data.error);
            setDevicesLoading(false);
            return;
          }

          const devices: DeviceInfo[] = data.devices || [];
          setAvailableDevices(devices);
          setDevicesLoaded(true);
          setDevicesLoading(false);

          console.log(`Loaded ${devices.length} devices:`, devices.map((d) => d.id).join(", "));
        }
      } catch (error) {
        console.error("Error parsing WebSocket message:", error);
      }
    };

    // We'll need to access the websocket instance directly
    // This will be handled in the main WebSocket hook
  });

  onCleanup(() => {
    messageHandler = null;
  });

  return {
    availableDevices,
    devicesLoaded,
    devicesLoading,
    requestDevices,
  };
}

// Export the message handler so it can be used by the WebSocket hook
export { messageHandler };
