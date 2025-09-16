import { createWSState, makeReconnectingWS } from "@solid-primitives/websocket";
import { createContext, createMemo, onCleanup, onMount, useContext } from "solid-js";
import { createStore } from "solid-js/store";
import { IWsReceiveMessage, IWsSendMessage } from "../interfaces/WebSockets";

// WebSocket Store Interface
export interface IWebSocketStore {
  // Connection state
  readyState: number;
  connectionStateName: string;
  isConnected: boolean;

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
  missedHeartbeats: number;
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
  
  // Heartbeat management
  sendHeartbeat: () => boolean;
  resetHeartbeat: () => void;
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

  // Create reconnecting WebSocket
  const websocket = makeReconnectingWS(wsUrl);
  const wsState = createWSState(websocket);

  // Message subscribers
  const messageSubscribers = new Set<MessageCallback>();
  
  // Heartbeat management
  let heartbeatTimer: ReturnType<typeof setInterval> | null = null;
  let heartbeatTimeoutTimer: ReturnType<typeof setTimeout> | null = null;
  const HEARTBEAT_INTERVAL = 30000; // 30 seconds
  const HEARTBEAT_TIMEOUT = 10000; // 10 seconds timeout for heartbeat response
  const MAX_MISSED_HEARTBEATS = 3;

  // Initialize store with default values
  const [store, setStore] = createStore<IWebSocketStore>(
    {
      // Connection state
      readyState: websocket.readyState,
      connectionStateName: wsStates[websocket.readyState] || "Disconnected",
      isConnected: false,

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
      heartbeatInterval: 30000, // 30 seconds
      missedHeartbeats: 0,
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
      missedHeartbeats: 0,
      lastHeartbeat: Date.now(),
    });
    
    // Start heartbeat
    startHeartbeat();
  };

  const handleClose = (event: CloseEvent) => {
    console.log("WebSocket connection closed", event.reason);
    setStore("isConnected", false);
    
    // Stop heartbeat
    stopHeartbeat();
  };

  const handleError = (event: Event) => {
    console.error("WebSocket error", event);
    setStore("error", "WebSocket connection error");
    setStore("reconnectAttempts", (prev) => prev + 1);
  };

  const handleMessage = (event: MessageEvent) => {
    const data = event.data;

    // Update message history
    setStore("lastMessage", data);
    setStore("lastMessages", (prev) => [...prev, data].slice(-20)); // Keep only last 20 messages

    console.log("WebSocket message received:", data);

    // Parse message and notify subscribers
    let parsedData: IWsReceiveMessage | undefined;
    try {
      parsedData = JSON.parse(data) as IWsReceiveMessage;
    } catch (error) {
      console.error("Non-JSON message received:", error);
    }

    if (parsedData) {
      // Handle heartbeat response
      if (parsedData.type === "pong" || parsedData.type === "heartbeat-response") {
        console.debug("Heartbeat response received");
        setStore({
          lastHeartbeat: Date.now(),
          missedHeartbeats: 0,
        });
        
        // Clear heartbeat timeout
        if (heartbeatTimeoutTimer) {
          clearTimeout(heartbeatTimeoutTimer);
          heartbeatTimeoutTimer = null;
        }
        return; // Don't notify subscribers for heartbeat messages
      }
      
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

  // Heartbeat functions
  const startHeartbeat = () => {
    stopHeartbeat(); // Clear any existing heartbeat
    
    heartbeatTimer = setInterval(() => {
      if (websocket.readyState === WebSocket.OPEN) {
        // Send heartbeat
        const heartbeatMessage = { type: "ping", timestamp: Date.now() };
        websocket.send(JSON.stringify(heartbeatMessage));
        console.debug("Heartbeat sent");
        
        // Set timeout for heartbeat response
        heartbeatTimeoutTimer = setTimeout(() => {
          const missed = store.missedHeartbeats + 1;
          setStore("missedHeartbeats", missed);
          
          console.warn(`Missed heartbeat ${missed}/${MAX_MISSED_HEARTBEATS}`);
          
          if (missed >= MAX_MISSED_HEARTBEATS) {
            console.error("Connection appears to be lost - too many missed heartbeats");
            setStore({
              error: "Connection lost - heartbeat timeout",
              isConnected: false,
            });
            
            // Force close and reconnect
            websocket.close();
          }
        }, HEARTBEAT_TIMEOUT);
      }
    }, HEARTBEAT_INTERVAL);
  };
  
  const stopHeartbeat = () => {
    if (heartbeatTimer) {
      clearInterval(heartbeatTimer);
      heartbeatTimer = null;
    }
    if (heartbeatTimeoutTimer) {
      clearTimeout(heartbeatTimeoutTimer);
      heartbeatTimeoutTimer = null;
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
      
      // Clean up heartbeat timers
      stopHeartbeat();
    });
  });

  // Actions object
  const actions: IWebSocketActions = {
    sendMessage: (message: IWsSendMessage): boolean => {
      if (websocket.readyState === WebSocket.OPEN) {
        console.debug("WebSocket message sent:", message);
        websocket.send(JSON.stringify(message));
        return true;
      } else {
        console.error("WebSocket is not open, cannot send message:", message);
        setStore({ error: "WebSocket is not connected" });
        return false;
      }
    },

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
    
    sendHeartbeat: (): boolean => {
      if (websocket.readyState === WebSocket.OPEN) {
        const heartbeatMessage = { type: "ping", timestamp: Date.now() };
        websocket.send(JSON.stringify(heartbeatMessage));
        console.debug("Manual heartbeat sent");
        return true;
      }
      return false;
    },
    
    resetHeartbeat: () => {
      setStore({
        lastHeartbeat: Date.now(),
        missedHeartbeats: 0,
      });
      
      if (websocket.readyState === WebSocket.OPEN) {
        startHeartbeat();
      }
    },
  };

  return [
    /** Don't destructure! */
    store,
    actions,
  ];
}

/**
 * Hook for subscribing to WebSocket messages
 * Automatically subscribes onMount and unsubscribes onCleanup
 *
 * @param callback Function to call when a message is received
 *
 * @example
 * ```tsx
 * function MyComponent() {
 *   const [messages, setMessages] = createSignal<string[]>([]);
 *
 *   useWebSocketMessage((rawMessage, parsedMessage) => {
 *     console.log("Received:", parsedMessage);
 *     if (parsedMessage?.type === "device-state") {
 *       setMessages(prev => [...prev, rawMessage]);
 *     }
 *   });
 *
 *   return <div>Messages: {messages().length}</div>;
 * }
 * ```
 */
export function useWebSocketMessage(callback: MessageCallback): void {
  const [, wsActions] = useWebSocket2();

  onMount(() => {
    const unsubscribe = wsActions.subscribe(callback);

    onCleanup(() => {
      unsubscribe();
    });
  });
}

/**
 * Hook for monitoring WebSocket connection health
 * Returns computed values for connection health status
 * 
 * @example
 * ```tsx
 * function ConnectionStatus() {
 *   const health = useWebSocketHealth();
 *   
 *   return (
 *     <div class={health.isHealthy ? "healthy" : "unhealthy"}>
 *       Status: {health.statusText}
 *       {health.lastHeartbeat && (
 *         <span>Last heartbeat: {new Date(health.lastHeartbeat).toLocaleTimeString()}</span>
 *       )}
 *     </div>
 *   );
 * }
 * ```
 */
export function useWebSocketHealth() {
  const [wsStore] = useWebSocket2();
  
  const isHealthy = createMemo(() => {
    return wsStore.isConnected && wsStore.missedHeartbeats < 2;
  });
  
  const statusText = createMemo(() => {
    if (!wsStore.isConnected) return "Disconnected";
    if (wsStore.missedHeartbeats === 0) return "Healthy";
    if (wsStore.missedHeartbeats < 2) return "Warning";
    return "Unhealthy";
  });
  
  const timeSinceLastHeartbeat = createMemo(() => {
    if (!wsStore.lastHeartbeat) return null;
    return Date.now() - wsStore.lastHeartbeat;
  });
  
  return {
    isHealthy: isHealthy(),
    statusText: statusText(),
    lastHeartbeat: wsStore.lastHeartbeat,
    timeSinceLastHeartbeat: timeSinceLastHeartbeat(),
    missedHeartbeats: wsStore.missedHeartbeats,
    isConnected: wsStore.isConnected,
    connectionStateName: wsStore.connectionStateName,
  };
}
