import { type Component, createSignal, onMount, onCleanup, createEffect, For } from "solid-js";
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
  const [availableSSIDs, setAvailableSSIDs] = createSignal<string[]>([]);
  const [availableNetworks, setAvailableNetworks] = createSignal<Array<{
    ssid: string;
    rssi: number;
    encryption: number;
    channel: number;
    bssid: string;
    hidden: boolean;
  }>>([]);
  const [loadingSSIDs, setLoadingSSIDs] = createSignal(false);

  // Computed signal for sorted networks (best signal first)
  const sortedNetworks = () => {
    return [...availableNetworks()].sort((a, b) => b.rssi - a.rssi);
  };

  // Function to convert RSSI to signal strength bars
  const getSignalStrength = (rssi: number) => {
    if (rssi >= -50) return { bars: 4, label: "Excellent" };
    if (rssi >= -60) return { bars: 3, label: "Good" };
    if (rssi >= -70) return { bars: 2, label: "Fair" };
    if (rssi >= -80) return { bars: 1, label: "Weak" };
    return { bars: 0, label: "Very Weak" };
  };

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
    // Load available SSIDs when entering edit mode
    loadAvailableSSIDs();
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

  // Load available SSIDs
  const loadAvailableSSIDs = () => {
    setLoadingSSIDs(true);
    sendMessage({ type: "get-networks" });
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
      } else if (message.type === "get-networks") {
        setLoadingSSIDs(false);
        if ("error" in message) {
          // Handle error - maybe just ignore or show a warning
          console.warn("Failed to load available networks:", message.error);
        } else {
          // Store full network objects and extract SSIDs for backward compatibility
          setAvailableNetworks(message.networks);
          const ssids = message.networks.map(network => network.ssid);
          setAvailableSSIDs(ssids);
        }
      }
    });

    onCleanup(() => {
      unsubscribe();
    });
  });

  // Load network config when popup opens
  createEffect(() => {
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
        {isEditing() ? (
          <div>
            <div style={{ "margin-bottom": "1rem" }}>
              <div style={{ display: "flex", "align-items": "center", "margin-bottom": "0.5rem" }}>
                <label style={{ display: "block", "margin-right": "0.5rem" }}>
                  <strong>SSID:</strong>
                </label>
                <button
                  onClick={loadAvailableSSIDs}
                  disabled={loadingSSIDs()}
                  style={{
                    padding: "0.25rem 0.5rem",
                    "font-size": "0.8rem",
                    border: "1px solid #ccc",
                    "border-radius": "3px",
                    background: "#f8f9fa",
                    cursor: loadingSSIDs() ? "not-allowed" : "pointer"
                  }}
                  title="Refresh available networks"
                >
                  {loadingSSIDs() ? "⟳" : "🔄"}
                </button>
              </div>
              {availableSSIDs().length > 0 && (
                <div style={{ "margin-bottom": "0.5rem" }}>
                  <select
                    onChange={(e) => {
                      const selectedSsid = e.currentTarget.value;
                      if (selectedSsid) {
                        setEditSsid(selectedSsid);
                      }
                      // Reset select to empty after selection
                      e.currentTarget.value = "";
                    }}
                    style={{
                      width: "100%",
                      padding: "0.5rem",
                      "border-radius": "4px",
                      border: "1px solid #ccc",
                      "font-size": "0.9rem",
                      "background-color": "#f8f9fa"
                    }}
                  >
                    <option value="">Select a network to autofill...</option>
                    <For each={sortedNetworks()}>
                      {(network) => {
                        const signal = getSignalStrength(network.rssi);
                        const bars = "█".repeat(signal.bars) + "░".repeat(4 - signal.bars);
                        return (
                          <option value={network.ssid}>
                            {bars} {network.ssid} ({signal.label})
                          </option>
                        );
                      }}
                    </For>
                  </select>
                </div>
              )}
              {loadingSSIDs() && availableSSIDs().length === 0 ? (
                <div style={{
                  width: "100%",
                  padding: "0.5rem",
                  "border-radius": "4px",
                  border: "1px solid #ccc",
                  "background-color": "#f8f9fa",
                  "text-align": "center",
                  color: "#666",
                  "margin-bottom": "0.5rem"
                }}>
                  🔄 Scanning for networks...
                </div>
              ) : null}
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
        ) : networkError() ? (
          <div style={{ color: "red" }}>
            <p>Error: {networkError()}</p>
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