import { createContext } from "solid-js";
import type { ParentComponent } from "solid-js";
import { getWebSocketContextValue, initializeWebSocket } from "./useWebSocket2";
import type { IWebSocketStore, IWebSocketActions } from "./useWebSocket2";

// WebSocket Context type
type WebSocketContextType = {
  store: IWebSocketStore;
  actions: IWebSocketActions;
} | undefined;

// Create WebSocket Context (re-export for external use)
export const WebSocketContext = createContext<WebSocketContextType>(undefined);

/**
 * WebSocket Provider Component
 * Creates a single WebSocket instance that can be shared across components
 * 
 * @example
 * ```tsx
 * function App() {
 *   return (
 *     <WebSocketProvider url="ws://localhost:8080/ws">
 *       <MyComponent />
 *       <AnotherComponent />
 *     </WebSocketProvider>
 *   );
 * }
 * ```
 */
export const WebSocketProvider: ParentComponent<{ url?: string }> = (props) => {
  // Initialize WebSocket on first render
  initializeWebSocket(props.url);
  
  // Get the initialized context value
  const contextValue = getWebSocketContextValue();
  
  return (
    <WebSocketContext.Provider value={contextValue}>
      {props.children}
    </WebSocketContext.Provider>
  );
};