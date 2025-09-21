import { type Component, createSignal, onMount, onCleanup } from "solid-js";
import { useWebSocket2 } from "../hooks/useWebSocket2";
import { Popup } from "./Popup";
import PopupHeader from "./PopupHeader";
import PopupContent from "./PopupContent";
import PopupFooter from "./PopupFooter";
import { IWsReceiveMessage } from "../interfaces/WebSockets";

interface NetworkConfigProps {
  isOpen: boolean;
  onClose: () => void;
}

export const NetworkConfig: Component<NetworkConfigProps> = (props) => {
  const [, { sendMessage, subscribe }] = useWebSocket2();
  const [networkInfo, setNetworkInfo] = createSignal<{ ssid: string; password: string } | null>(null);
  const [networkError, setNetworkError] = createSignal<string | null>(null);
  const [isEditing, setIsEditing] = createSignal(false);
  const [editSsid, setEditSsid] = createSignal("");
  const [editPassword, setEditPassword] = createSignal("");

  // Configure button handler
  const handleConfigure = () => {
    if (networkInfo()) {
      setEditSsid(networkInfo()!.ssid);
      setEditPassword(networkInfo()!.password || "");
    } else {
      // No existing config, start with empty fields
      setEditSsid("");
      setEditPassword("");
    }
    setIsEditing(true);
  };

  // Save configuration handler
  const handleSaveConfig = () => {
    sendMessage({
      type: "set-network-config",
      ssid: editSsid(),
      password: editPassword(),
    } as any);
    setIsEditing(false);
    // Refresh network config after saving
    setTimeout(() => {
      sendMessage({ type: "get-network-config" });
    }, 1000);
  };

  // Cancel configuration handler
  const handleCancelConfig = () => {
    setIsEditing(false);
  };

  // Subscribe to network config messages
  onMount(() => {
    const unsubscribe = subscribe((message: IWsReceiveMessage) => {
      if (message.type === "get-network-config") {
        if ("error" in message) {
          setNetworkError(message.error);
        } else {
          setNetworkInfo({ ssid: message.ssid, password: message.password });
        }
      } else if (message.type === "set-network-config") {
        if ("error" in message) {
          setNetworkError(message.error);
        } else {
          // Success - the network config was updated
          setNetworkError(null);
        }
      }
    });

    onCleanup(() => {
      unsubscribe();
    });
  });

  // Load network config when popup opens
  onMount(() => {
    if (props.isOpen) {
      setNetworkInfo(null);
      setNetworkError(null);
      setIsEditing(false);
      sendMessage({ type: "get-network-config" });
    }
  });

  return (
    <Popup isOpen={props.isOpen}>
      <PopupHeader title="Network Configuration">
        <button
          onClick={props.onClose}
          style={{ background: "none", border: "none", "font-size": "24px", cursor: "pointer" }}
        >
          ×
        </button>
      </PopupHeader>
      <PopupContent>
        {networkError() ? (
          <div style={{ color: "red" }}>
            <p>Error: {networkError()}</p>
          </div>
        ) : isEditing() ? (
          <div>
            <div style={{ "margin-bottom": "1rem" }}>
              <label style={{ display: "block", "margin-bottom": "0.5rem" }}>
                <strong>SSID:</strong>
              </label>
              <input
                type="text"
                value={editSsid()}
                onInput={(e) => setEditSsid(e.currentTarget.value)}
                placeholder="Enter WiFi network name"
                style={{
                  width: "100%",
                  padding: "0.5rem",
                  "border-radius": "4px",
                  border: "1px solid #ccc",
                  "font-size": "1rem"
                }}
              />
            </div>
            <div>
              <label style={{ display: "block", "margin-bottom": "0.5rem" }}>
                <strong>Password:</strong>
              </label>
              <input
                type="password"
                value={editPassword()}
                onInput={(e) => setEditPassword(e.currentTarget.value)}
                placeholder="Enter WiFi password (optional)"
                style={{
                  width: "100%",
                  padding: "0.5rem",
                  "border-radius": "4px",
                  border: "1px solid #ccc",
                  "font-size": "1rem"
                }}
              />
            </div>
          </div>
        ) : networkInfo() ? (
          <div>
            <p><strong>SSID:</strong> {networkInfo()?.ssid}</p>
            <p><strong>Password:</strong> {networkInfo()?.password ? "••••••••" : "Not set"}</p>
          </div>
        ) : (
          <div>
            <p>Loading network configuration...</p>
          </div>
        )}
      </PopupContent>
      <PopupFooter>
        {isEditing() ? (
          <>
            <button onClick={handleCancelConfig}>Cancel</button>
            <button
              onClick={handleSaveConfig}
              style={{ "margin-left": "0.5rem", "background-color": "#007bff", color: "white" }}
            >
              Save
            </button>
          </>
        ) : (
          <>
            <button onClick={handleConfigure}>
              Configure
            </button>
            <button
              onClick={props.onClose}
              style={{ "margin-left": "0.5rem" }}
            >
              Close
            </button>
          </>
        )}
      </PopupFooter>
    </Popup>
  );
};