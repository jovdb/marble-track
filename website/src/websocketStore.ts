import { createStore } from "solid-js/store";
import { createEffect, onCleanup } from "solid-js";
import { WebSocketManager, ConnectionStatus, WebSocketMessage, createWebSocket } from "./websocket";

export interface WebSocketState {
  status: ConnectionStatus;
  lastMessage: WebSocketMessage | null;
  lastError: Event | null;
  isConnected: boolean;
  messages: WebSocketMessage[];
  reconnectAttempts: number;
}

export interface WebSocketStore {
  state: WebSocketState;
  connect: (url: string) => void;
  disconnect: () => void;
  send: (message: WebSocketMessage) => boolean;
  clearMessages: () => void;
  clearError: () => void;
}

const initialState: WebSocketState = {
  status: 'disconnected',
  lastMessage: null,
  lastError: null,
  isConnected: false,
  messages: [],
  reconnectAttempts: 0,
};

export function createWebSocketStore(): WebSocketStore {
  const [state, setState] = createStore<WebSocketState>(initialState);
  let wsManager: WebSocketManager | null = null;

  // Auto-connect on store creation
  const wsUrl = `ws://${window.location.hostname}/ws`;

  // Setup WebSocket event handlers
  const setupWebSocketHandlers = (manager: WebSocketManager) => {
    manager.onStatusChange((status: ConnectionStatus) => {
      setState({
        status,
        isConnected: status === 'connected',
        reconnectAttempts: status === 'connected' ? 0 : state.reconnectAttempts + (status === 'connecting' ? 1 : 0),
      });
    });

    manager.onMessage((message: WebSocketMessage) => {
      setState({
        lastMessage: message,
        messages: [...state.messages, message].slice(-100), // Keep last 100 messages
      });
    });

    manager.onError((error: Event) => {
      setState({
        lastError: error,
      });
    });
  };

  const connect = (url: string) => {
    // Disconnect existing connection if any
    if (wsManager) {
      wsManager.disconnect();
    }

    // Create new WebSocket connection
    wsManager = createWebSocket({
      url,
      reconnectInterval: 3000,
      maxReconnectAttempts: 5,
      heartbeatInterval: 30000,
    });

    setupWebSocketHandlers(wsManager);
    wsManager.connect();
  };

  const disconnect = () => {
    if (wsManager) {
      wsManager.disconnect();
      wsManager = null;
    }
  };

  const send = (message: WebSocketMessage): boolean => {
    if (wsManager) {
      return wsManager.send(message);
    }
    console.warn('WebSocket not connected');
    return false;
  };

  const clearMessages = () => {
    setState({
      messages: [],
      lastMessage: null,
    });
  };

  const clearError = () => {
    setState({
      lastError: null,
    });
  };

  // Cleanup on component unmount
  onCleanup(() => {
    disconnect();
  });

  // Auto-connect when store is created
  connect(wsUrl);

  return {
    state,
    connect,
    disconnect,
    send,
    clearMessages,
    clearError,
  };
}

// Global WebSocket store instance (optional - use if you want a singleton)
export const webSocketStore = createWebSocketStore();
