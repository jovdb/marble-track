import {
  createWSState,
  makeHeartbeatWS,
  makeReconnectingWS,
  makeWS,
} from "@solid-primitives/websocket";

import { createMemo, createSignal, onCleanup, onMount } from "solid-js";

const marbleIp = import.meta.env.VITE_MARBLE_IP || window.location.hostname;
const url = `ws://${marbleIp}/ws`;

// const websocket = makeHeartbeatWS(makeReconnectingWS(url), {
//   message: "👍", // heartbeat message
// });
const websocket = makeReconnectingWS(url);

const wsState = createWSState(websocket);
const wsStates = [
  "Connecting",
  "Connected",
  "Disconnecting",
  "Disconnected",
  "Fetching", // Not standard
  "Fetched", // Not standard
] as const;

export const connectionStateName = createMemo(() => wsStates[wsState()], 0);
export const isConnected = createMemo(() => wsState() === 1); // Connected state

export const [lastMessage, setLastMessage] = createSignal<string | null>(null);
export const [lastMessages, setLastMessages] = createSignal<string[]>([]);

websocket.addEventListener("open", () => {
  console.log("WebSocket connection opened");
});
websocket.addEventListener("close", (e) => {
  console.log("WebSocket connection closed", e.reason);
});
websocket.addEventListener("error", (e) => {
  console.error("WebSocket error", e);
});
websocket.addEventListener("message", (e) => {
  console.log("WebSocket message received:", e.data);

  const data = e.data;
  setLastMessage(data);

  setLastMessages((prev) => {
    const newArray = [...prev, data];
    newArray.slice(-20);
    return newArray;
  });
});

export const clearMessages = () => setLastMessages([]);

export const sendMessage = (message: string): boolean => {
  if (websocket.readyState === WebSocket.OPEN) {
    console.debug(" WebSocket message sent: ", message);
    websocket.send(message);
    return true;
  } else {
    console.error("WebSocket is not open, cannot send message: ", message);
    return false;
  }
};

export function createDeviceState<T>(deviceId: string) {
  // Get initial value
  const [deviceState, setDeviceState] = createSignal<T | undefined>(undefined);
  const [connectedState, setConnectionState] = createSignal<number>(
    websocket.readyState
  );

  onMount(() => {
    // Is connected, request state
    if (websocket.readyState === WebSocket.OPEN) {
      setConnectionState(4); // Fetching state
      sendMessage(
        JSON.stringify({
          action: "device-state",
          deviceId,
        })
      );
    }

    function onMessage(event: MessageEvent) {
      const data = JSON.parse(event.data);
      if (
        typeof data === "object" &&
        data.type === "device-state" &&
        data.deviceId === deviceId
      ) {
        setDeviceState(data.state);
        setConnectionState(5);
      }
    }

    function onOpen(event: Event) {
      // Request state
      setConnectionState(4); // Fetching state
      sendMessage(
        JSON.stringify({
          action: "device-state",
          deviceId,
        })
      );
    }

    function onClose(event: Event) {
      setConnectionState(websocket.readyState);
    }

    // listen to state changed
    websocket.addEventListener("message", onMessage);

    // If WS is not connected and it WS becomes connected
    websocket.addEventListener("open", onOpen);

    // If WS is not connected and it WS becomes connected
    websocket.addEventListener("close", onClose);

    onCleanup(() => {
      // Clean up the event listener when the component is unmounted
      websocket.removeEventListener("open", onOpen);
      websocket.removeEventListener("message", onMessage);
    });
  });

  const connectionState = createMemo(() => wsStates[connectedState()]);
  const disabled = createMemo(() => connectionState() !== "Fetched");

  return [deviceState, connectionState, disabled] as const;
}
