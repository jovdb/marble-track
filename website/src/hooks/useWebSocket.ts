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
] as const;

export const connectionStateName = createMemo(() => wsStates[wsState()], 0);
export const isConnected = createMemo(() => wsState() === 1); // Connected state

export const [lastMessage, setLastMessage] = createSignal<string | null>(null);
export const [lastMessages, setLastMessages] = createSignal<
  { timestamp: number; message: string }[]
>([]);

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
    console.debug("Sending WebSocket message:", message);
    websocket.send(message);
    return true;
  } else {
    console.error("WebSocket is not open, cannot send message");
    return false;
  }
};

export function createDeviceState<T>(deviceId: string) {
  // Get initial value
  const [state, setState] = createSignal<T | undefined>(undefined);
  onMount(() => {
    // TODO, Get initial value
    function onMessage(event: MessageEvent) {
      const data = JSON.parse(event.data);
      if (
        typeof data === "object" &&
        data.type === "state-changed" &&
        data.deviceId === deviceId
      ) {
        setState(data.state);
      }
    }

    websocket.addEventListener("message", onMessage);

    onCleanup(() => {
      // Clean up the event listener when the component is unmounted
      websocket.removeEventListener("message", onMessage);
    });
  });

  return state;
}
/*
export function createWebSocket(logger?: ILogger) {
  // Listen for messages
  createEventListener(websocket, "message", (event) => {
    // const data = JSON.parse(event.data);
    const data = event.data;
    logger?.debug("Received WebSocket message:", data);
    setLastMessage(data);
    setLastMessages((prev) => {
      const newArray = [...prev, data];
      newArray.slice(-20);
      return newArray;
    });
  });

  return {
    sendMessage: (message: string) => {
      logger?.debug("Sending WebSocket message:", message);
      websocket.send(message);
    },
    isConnected: () => wsState() === 2, // Connected state
    status: connectionStateName,
    lastMessage,
    lastMessages,
    clearMessages: () => setLastMessages([]),
  };
}
*/
