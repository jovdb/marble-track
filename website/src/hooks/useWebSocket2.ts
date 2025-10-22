import { createWSState, makeReconnectingWS } from "@solid-primitives/websocket";
import { createContext, createMemo, onCleanup, onMount, useContext } from "solid-js";
import { createStore } from "solid-js/store";
import { IWsReceiveMessage, IWsSendMessage } from "../interfaces/WebSockets";
import { pipe } from "../utils/pipe";

// WebSocket Store Interface
export interface IWebSocketStore {
  // Connection state
  readyState: number;
  connectionStateName: string;
  isConnected: boolean;

  // Connection details
  url: string;

  // Messages
  lastMessage: string | null;
  lastMessages: string[];

  // Error handling
  error: string | null;

  // Reconnection
  reconnectAttempts: number;
  maxReconnectAttempts: number;
  reconnectDelay: number;

  // Heartbeat
  lastHeartbeat: number | null;
  heartbeatInterval: number;
  isHeartbeatEnabled: boolean;
}

// Message with direction type
export interface IWebSocketMessage {
  data: string;
  direction: "incoming" | "outgoing";
  timestamp: number;
}

// Message callback type
export type MessageCallback = (message: IWsReceiveMessage) => void;

// WebSocket Actions Interface
export interface IWebSocketActions {
  // Message handling
  sendMessage: (message: IWsSendMessage) => boolean;
  clearMessages: () => void;

  // Message subscription
  subscribe: (callback: MessageCallback) => () => void;

  // Connection management
  connect: () => void;
  disconnect: () => void;

  // State management
  setError: (error: string | null) => void;
  resetReconnectAttempts: () => void;
}

const wsStates = [
  "Connecting",
  "Connected",
  "Disconnecting",
  "Disconnected",
  "Fetching",
  "Fetched",
  "Error",
] as const;

// WebSocket Context type
type WebSocketContextType =
  | {
      store: IWebSocketStore;
      actions: IWebSocketActions;
    }
  | undefined;

// Create WebSocket Context
const WebSocketContext = createContext<WebSocketContextType>(undefined);

// Global WebSocket instance - created once and shared
let globalWebSocketInstance: { store: IWebSocketStore; actions: IWebSocketActions } | null = null;

/**
 * Initialize the global WebSocket instance
 * This should be called once at app startup
 */
export function initializeWebSocket(url?: string): void {
  if (globalWebSocketInstance) {
    console.warn("WebSocket already initialized. Ignoring duplicate initialization.");
    return;
  }

  const [store, actions] = createWebSocketStore(url);
  globalWebSocketInstance = { store, actions };
}

/**
 * Get the WebSocket context value for provider
 * Must be called after initializeWebSocket
 */
export function getWebSocketContextValue(): WebSocketContextType {
  if (!globalWebSocketInstance) {
    throw new Error("WebSocket not initialized. Call initializeWebSocket() first.");
  }

  return globalWebSocketInstance;
}

/**
 * Pure WebSocket hook that uses a shared WebSocket instance via context
 * Must be used within a WebSocketProvider or after calling initializeWebSocket()
 * Returns a tuple with WebSocket state store and actions object
 *
 * @example
 * ```tsx
 * // At app startup:
 * initializeWebSocket("ws://localhost:8080/ws");
 *
 * // In components:
 * function MyComponent() {
 *   const [wsStore, wsActions] = useWebSocket2();
 *   const [messages, setMessages] = createSignal<string[]>([]);
 *
 *   createEffect(() => {
 *     if (wsStore.isConnected) {
 *       wsActions.sendMessage({ type: "ping" });
 *     }
 *   });
 *
 *   // Manual subscription (or use useWebSocketMessage hook)
 *   onMount(() => {
 *     const unsubscribe = wsActions.subscribe((rawMessage, parsedMessage) => {
 *       setMessages(prev => [...prev, rawMessage]);
 *     });
 *
 *     onCleanup(unsubscribe);
 *   });
 *
 *   return (
 *     <div>
 *       <p>Status: {wsStore.connectionStateName}</p>
 *       <p>Messages: {wsStore.lastMessages.length}</p>
 *       <p>Custom Messages: {messages().length}</p>
 *       <button onClick={() => wsActions.clearMessages()}>
 *         Clear Messages
 *       </button>
 *     </div>
 *   );
 * }
 * ```
 */
export function useWebSocket2(): [IWebSocketStore, IWebSocketActions] {
  // Try to get from context first
  const context = useContext(WebSocketContext);

  if (context) {
    return [context.store, context.actions];
  }

  // Fallback to global instance
  if (globalWebSocketInstance) {
    return [globalWebSocketInstance.store, globalWebSocketInstance.actions];
  }

  throw new Error(
    "useWebSocket2 must be used within a WebSocketProvider or after calling initializeWebSocket()"
  );
}

/**
 * Creates a WebSocket store instance
 * Internal function used by the context provider and global instance
 */
function createWebSocketStore(url?: string): [IWebSocketStore, IWebSocketActions] {
  // Use provided URL or fallback to environment/default
  const wsUrl = url || import.meta.env.VITE_MARBLE_WS || `ws://${window.location.hostname}/ws`;
  console.log("Connecting to WebSocket:", wsUrl);

  // Create reconnecting WebSocket with heartbeat
  const reconnectingWS = makeReconnectingWS(wsUrl);
  /*const websocket = makeHeartbeatWS(reconnectingWS, {
    message: JSON.stringify({ type: "ping" }),
    interval: 10000, // 10 seconds
    wait: 5000, // 10 seconds wait for response
  });*/
  const websocket = reconnectingWS;
  const wsState = createWSState(websocket);

  // Message subscribers
  const messageSubscribers = new Set<MessageCallback>();

  // Initialize store with default values
  const [store, setStore] = createStore<IWebSocketStore>(
    {
      // Connection state
      readyState: websocket.readyState,
      connectionStateName: wsStates[websocket.readyState] || "Disconnected",
      isConnected: false,

      // Connection details
      url: wsUrl,

      // Messages
      lastMessage: null,
      lastMessages: [],

      // Error handling
      error: null,

      // Reconnection
      reconnectAttempts: 0,
      maxReconnectAttempts: 5,
      reconnectDelay: 1000,

      // Heartbeat
      lastHeartbeat: null,
      heartbeatInterval: 30000,
      isHeartbeatEnabled: true,
    },
    { name: "useWebSocket2Store" }
  );

  // Create memos for computed values
  const connectionStateName = createMemo(() => wsStates[wsState()] || "Disconnected");
  const isConnected = createMemo(() => wsState() === WebSocket.OPEN);

  // Update store when WebSocket state changes
  createMemo(() => {
    setStore({
      readyState: wsState(),
      connectionStateName: connectionStateName(),
      isConnected: isConnected(),
    });
  });

  // Event handlers
  const handleOpen = () => {
    console.log("WebSocket connection opened");
    setStore({
      error: null,
      reconnectAttempts: 0,
      lastHeartbeat: Date.now(),
    });
  };

  const handleClose = (event: CloseEvent) => {
    console.log("WebSocket connection closed", event.reason);
    setStore("isConnected", false);
  };

  const handleError = (event: Event) => {
    console.error("WebSocket error", event);
    setStore("error", "WebSocket connection error");
    setStore("reconnectAttempts", (prev) => prev + 1);
  };

  const handleMessage = (event: MessageEvent) => {
    const data = event.data;

    // Parse message first to check for heartbeat responses
    let parsedData: IWsReceiveMessage | undefined;
    try {
      parsedData = JSON.parse(data) as IWsReceiveMessage;
    } catch (error) {
      console.error("Non-JSON message received:", error);
    }

    // Handle heartbeat responses (pong)
    if (parsedData && parsedData.type === "pong") {
      setStore("lastHeartbeat", Date.now());
      console.debug("Heartbeat pong received");
      // Don't add heartbeat messages to message history
      return;
    }

    // Update message history for non-heartbeat messages
    const message: IWebSocketMessage = {
      data,
      direction: "incoming",
      timestamp: Date.now(),
    };
    setStore("lastMessage", data);
    setStore("lastMessages", (prev) => [...prev, JSON.stringify(message)].slice(-20)); // Keep only last 20 messages

    console.log("WebSocket message received:", data);

    if (parsedData) {
      // Notify all subscribers
      messageSubscribers.forEach((callback) => {
        try {
          callback(parsedData);
        } catch (error) {
          console.error("Error in message subscriber callback:", error);
        }
      });
    }
  };

  // Set up event listeners
  onMount(() => {
    websocket.addEventListener("open", handleOpen);
    websocket.addEventListener("close", handleClose);
    websocket.addEventListener("error", handleError);
    websocket.addEventListener("message", handleMessage);

    onCleanup(() => {
      websocket.removeEventListener("open", handleOpen);
      websocket.removeEventListener("close", handleClose);
      websocket.removeEventListener("error", handleError);
      websocket.removeEventListener("message", handleMessage);
    });
  });

  const sendMessage = (message: IWsSendMessage): boolean => {
    if (websocket.readyState === WebSocket.OPEN) {
      const messageData = JSON.stringify(message);
      console.debug("WebSocket message sent:", message);
      websocket.send(messageData);

      // Add outgoing message to history
      const outgoingMessage: IWebSocketMessage = {
        data: messageData,
        direction: "outgoing",
        timestamp: Date.now(),
      };
      setStore("lastMessages", (prev) => [...prev, JSON.stringify(outgoingMessage)].slice(-20));

      return true;
    } else {
      console.error("WebSocket is not open, cannot send message:", message);
      setStore({ error: "WebSocket is not connected" });
      return false;
    }
  };

  // Actions object
  const actions: IWebSocketActions = {
    // TODO: deduplicate calls
    sendMessage: pipe(sendMessage),

    clearMessages: () => {
      setStore("lastMessages", []);
      setStore("lastMessage", null);
    },

    connect: () => {
      // For reconnecting WebSocket, we don't manually connect
      // The reconnecting logic handles this automatically
      console.log("Connection will be handled automatically by reconnecting WebSocket");
    },

    disconnect: () => {
      websocket.close();
    },

    setError: (error: string | null) => {
      setStore({ error });
    },

    resetReconnectAttempts: () => {
      setStore({ reconnectAttempts: 0 });
    },

    subscribe: (callback: MessageCallback) => {
      messageSubscribers.add(callback);

      // Return unsubscribe function
      return () => {
        messageSubscribers.delete(callback);
      };
    },
  };

  return [
    /** Don't destructure! */
    store,
    actions,
  ];
}

/**
 * Hook for monitoring WebSocket heartbeat health
 * Returns a signal indicating if the connection is healthy based on heartbeat
 *
 * @param timeoutMs Maximum time since last heartbeat before considering unhealthy (default: 60000ms = 1 minute)
 *
 * @example
 * ```tsx
 * function ConnectionStatus() {
 *   const [wsStore] = useWebSocket2();
 *   const isHealthy = useWebSocketHealth();
 *
 *   return (
 *     <div>
 *       Connection: {wsStore.isConnected ? "Connected" : "Disconnected"}
 *       Health: {isHealthy() ? "Healthy" : "Unhealthy"}
 *     </div>
 *   );
 * }
 * ```
 */
export function useWebSocketHealth(timeoutMs: number = 60000) {
  const [wsStore] = useWebSocket2();

  return createMemo(() => {
    if (!wsStore.isConnected || !wsStore.isHeartbeatEnabled) {
      return false;
    }

    if (wsStore.lastHeartbeat === null) {
      return false;
    }

    const timeSinceLastHeartbeat = Date.now() - wsStore.lastHeartbeat;
    return timeSinceLastHeartbeat < timeoutMs;
  });
}
