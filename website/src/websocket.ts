export type ConnectionStatus =
  | "disconnected"
  | "connecting"
  | "connected"
  | "error";

export interface WebSocketMessage {
  type: string;
  data: any;
  timestamp?: number;
}

export interface WebSocketConfig {
  url: string;
  protocols?: string | string[];
  reconnectInterval?: number;
  maxReconnectAttempts?: number;
  heartbeatInterval?: number;
}

export class WebSocketManager {
  private ws: WebSocket | null = null;
  private config: Required<WebSocketConfig>;
  private reconnectAttempts = 0;
  private reconnectTimer: number | null = null;
  private heartbeatTimer: number | null = null;
  private statusCallback: ((status: ConnectionStatus) => void) | null = null;
  private messageCallback: ((message: WebSocketMessage) => void) | null = null;
  private errorCallback: ((error: Event) => void) | null = null;

  constructor(config: WebSocketConfig) {
    this.config = {
      url: config.url,
      protocols: config.protocols || [],
      reconnectInterval: config.reconnectInterval || 3000,
      maxReconnectAttempts: config.maxReconnectAttempts || 5,
      heartbeatInterval: config.heartbeatInterval || 30000,
    };
    console.log("WebSocketManager created with config:", this.config);
  }

  connect(): void {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      console.warn("WebSocket is already connected");
      return;
    }

    console.log(`Attempting to connect to WebSocket: ${this.config.url}`);
    this.updateStatus("connecting");

    try {
      this.ws = new WebSocket(this.config.url, this.config.protocols);
      this.setupEventListeners();
    } catch (error) {
      console.error("Failed to create WebSocket connection:", error);
      this.updateStatus("error");
    }
  }

  disconnect(): void {
    console.log("Manually disconnecting WebSocket");
    this.clearTimers();
    this.reconnectAttempts = 0;

    if (this.ws) {
      this.ws.close(1000, "Manual disconnect");
      this.ws = null;
    }

    this.updateStatus("disconnected");
  }

  send(message: WebSocketMessage): boolean {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      console.warn("WebSocket is not connected, cannot send message:", message);
      return false;
    }

    try {
      const messageWithTimestamp = {
        ...message,
        timestamp: Date.now(),
      };
      console.log("Sending WebSocket message:", messageWithTimestamp);
      this.ws.send(JSON.stringify(messageWithTimestamp));
      return true;
    } catch (error) {
      console.error("Failed to send message:", error);
      return false;
    }
  }

  onStatusChange(callback: (status: ConnectionStatus) => void): void {
    this.statusCallback = callback;
  }

  onMessage(callback: (message: WebSocketMessage) => void): void {
    this.messageCallback = callback;
  }

  onError(callback: (error: Event) => void): void {
    this.errorCallback = callback;
  }

  getStatus(): ConnectionStatus {
    if (!this.ws) return "disconnected";

    switch (this.ws.readyState) {
      case WebSocket.CONNECTING:
        return "connecting";
      case WebSocket.OPEN:
        return "connected";
      case WebSocket.CLOSING:
      case WebSocket.CLOSED:
        return "disconnected";
      default:
        return "error";
    }
  }

  private setupEventListeners(): void {
    if (!this.ws) return;

    this.ws.onopen = (event) => {
      console.log("WebSocket connected successfully to:", this.config.url);
      this.reconnectAttempts = 0;
      this.updateStatus("connected");
      this.startHeartbeat();
    };

    this.ws.onclose = (event) => {
      console.log(
        "WebSocket disconnected - Code:",
        event.code,
        "Reason:",
        event.reason || "No reason provided"
      );
      this.clearTimers();

      if (
        event.code !== 1000 &&
        this.reconnectAttempts < this.config.maxReconnectAttempts
      ) {
        console.log("Non-normal close detected, scheduling reconnection...");
        this.scheduleReconnect();
      } else {
        if (event.code !== 1000) {
          console.log("Max reconnection attempts reached, giving up");
        }
        this.updateStatus("disconnected");
      }
    };

    this.ws.onerror = (event) => {
      console.error("WebSocket error occurred:", event);
      this.updateStatus("error");
      this.errorCallback?.(event);
    };

    this.ws.onmessage = (event) => {
      console.log("Received WebSocket message:", event.data);
      try {
        const message: WebSocketMessage = JSON.parse(event.data);
        console.log("Received WebSocket message:", message);
        this.messageCallback?.(message);
      } catch (error) {
        console.error(
          "Failed to parse WebSocket message:",
          error,
          "Raw data:",
          event.data
        );
      }
    };
  }

  private scheduleReconnect(): void {
    this.reconnectAttempts++;
    this.updateStatus("connecting");

    console.log(
      `Scheduling reconnection attempt ${this.reconnectAttempts}/${this.config.maxReconnectAttempts} in ${this.config.reconnectInterval}ms...`
    );

    this.reconnectTimer = window.setTimeout(() => {
      console.log(`Executing reconnection attempt ${this.reconnectAttempts}`);
      this.connect();
    }, this.config.reconnectInterval);
  }

  private startHeartbeat(): void {
    console.log(
      `Starting heartbeat with interval: ${this.config.heartbeatInterval}ms`
    );
    this.heartbeatTimer = window.setInterval(() => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        console.log("Sending heartbeat ping");
        this.send({ type: "ping", data: null });
      } else {
        console.warn(
          "Heartbeat attempted but WebSocket not open, clearing heartbeat timer"
        );
        this.clearTimers();
      }
    }, this.config.heartbeatInterval);
  }

  private clearTimers(): void {
    if (this.reconnectTimer) {
      console.log("Clearing reconnection timer");
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }

    if (this.heartbeatTimer) {
      console.log("Clearing heartbeat timer");
      clearInterval(this.heartbeatTimer);
      this.heartbeatTimer = null;
    }
  }

  private updateStatus(status: ConnectionStatus): void {
    console.log(`WebSocket status changed to: ${status}`);
    this.statusCallback?.(status);
  }
}

// Factory function for creating WebSocket instances
export function createWebSocket(config: WebSocketConfig): WebSocketManager {
  console.log("Creating new WebSocket instance with config:", config);
  return new WebSocketManager(config);
}
