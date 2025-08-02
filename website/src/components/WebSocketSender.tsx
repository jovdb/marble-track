import { type Component, createSignal } from "solid-js";
import { webSocketStore } from "../stores/websocketStore";

const WebSocketSender: Component = () => {
  const [messageData, setMessageData] = createSignal("");

  const sendMessage = () => {
    if (!messageData() || !webSocketStore.state.isConnected) {
      return;
    }

    let message: any;
    
    try {
      // Try to parse as JSON first
      message = JSON.parse(messageData());
    } catch (error) {
      // If JSON parsing fails, send as a simple message with raw data
      message = {
        type: "raw_message",
        data: messageData()
      };
    }

    const success = webSocketStore.send(message);
    
    if (success) {
      // Clear the form after successful send
      setMessageData("");
    }
  };

  const handleKeyPress = (e: KeyboardEvent) => {
    if (e.key === 'Enter' && e.ctrlKey) {
      sendMessage();
    }
  };

  return (
    <fieldset>
      <legend>Send WebSocket Message</legend>
      
      <div style={{ "margin-bottom": "10px" }}>
        <textarea
          value={messageData()}
          onInput={(e) => setMessageData((e.target as HTMLTextAreaElement).value)}
          onKeyDown={handleKeyPress}
          placeholder='{"type": "ping", "data": {}} or just plain text'
          rows={3}
          style={{
            width: "100%",
            padding: "8px",
            border: "1px solid #ccc",
            "border-radius": "4px",
            "font-family": "monospace",
            resize: "vertical",
            "box-sizing": "border-box"
          }}
        />
        <small style={{ color: "#666", "font-size": "12px" }}>
          Enter JSON object or plain text. Press Ctrl+Enter to send.
        </small>
      </div>

      <button
        onClick={sendMessage}
        disabled={!webSocketStore.state.isConnected || !messageData()}
        style={{
          padding: "8px 16px",
          "background-color": webSocketStore.state.isConnected ? "#2563eb" : "#9ca3af",
          color: "white",
          border: "none",
          "border-radius": "4px",
          cursor: webSocketStore.state.isConnected ? "pointer" : "not-allowed",
          "font-weight": "bold"
        }}
      >
        Send
      </button>

      {!webSocketStore.state.isConnected && (
        <p style={{ color: "#ef4444", "margin-top": "10px", "font-size": "12px" }}>
          WebSocket is not connected.
        </p>
      )}
    </fieldset>
  );
};

export default WebSocketSender;
