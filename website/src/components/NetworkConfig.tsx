import { type Component, createSignal, onMount, onCleanup, createEffect, For } from "solid-js";
import { useWebSocket2 } from "../hooks/useWebSocket";
import { Popup } from "./Popup";
import PopupHeader from "./PopupHeader";
import PopupContent from "./PopupContent";
import PopupFooter from "./PopupFooter";

interface NetworkConfigProps {
  isOpen: boolean;
  onClose: () => void;
}

export const NetworkConfig: Component<NetworkConfigProps> = (props) => {
  const [, { sendMessage, subscribe }] = useWebSocket2();
  const [networkInfo, setNetworkInfo] = createSignal<{ ssid: string; password: string } | null>(
    null
  );
  const [networkStatus, setNetworkStatus] = createSignal<{
    mode: string;
    connected: boolean;
    ssid: string;
    ip: string;
    rssi?: number;
    clients?: number;
  } | null>(null);
  const [networkError, setNetworkError] = createSignal<string | null>(null);
  const [isEditing, setIsEditing] = createSignal(false);
  const [editSsid, setEditSsid] = createSignal("");
  const [editPassword, setEditPassword] = createSignal("");
  const [availableSSIDs, setAvailableSSIDs] = createSignal<string[]>([]);
  const [availableNetworks, setAvailableNetworks] = createSignal<
    Array<{
      ssid: string;
      rssi: number;
      encryption: number;
      channel: number;
      bssid: string;
      hidden: boolean;
    }>
  >([]);
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
      sendMessage({ type: "network-config" });
    }, 1000);
  };

  // Cancel configuration handler
  const handleCancelConfig = () => {
    setIsEditing(false);
  };

  // Load available SSIDs
  const loadAvailableSSIDs = () => {
    setLoadingSSIDs(true);
    sendMessage({ type: "networks" });
  };

  // Subscribe to network config messages
  onMount(() => {
    const unsubscribe = subscribe((message) => {
      if (message.type === "network-config") {
        if ("error" in message) {
          setNetworkError(message.error);
        } else {
          setNetworkInfo({ ssid: message.ssid, password: "" }); // Password not sent for security
        }
      } else if (message.type === "set-network-config") {
        if ("error" in message) {
          setNetworkError(message.error);
        } else {
          // Success - the network config was updated
          setNetworkError(null);
        }
      } else if (message.type === "networks") {
        setLoadingSSIDs(false);
        if ("error" in message) {
          // Handle error - maybe just ignore or show a warning
          console.warn("Failed to load available networks:", message.error);
        } else {
          // Store full network objects and extract SSIDs for backward compatibility
          setAvailableNetworks(message.networks);
          const ssids = message.networks.map((network) => network.ssid);
          setAvailableSSIDs(ssids);
        }
      } else if (message.type === "network-status") {
        if ("error" in message) {
          console.warn("Failed to load network status:", message.error);
        } else {
          setNetworkStatus(message.status);
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
      setNetworkStatus(null);
      setNetworkError(null);
      setIsEditing(false);
      sendMessage({ type: "network-config" });
      sendMessage({ type: "network-status" });
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
        {/* Network Status Section - only show when not editing */}
        {!isEditing() && (
          <div
            style={{
              "margin-bottom": "1rem",
              padding: "1rem",
              "background-color": "#f8f9fa",
              "border-radius": "4px",
            }}
          >
            <h3 style={{ margin: "0 0 0.5rem 0", "font-size": "1.1rem" }}>Network Status</h3>
            {networkStatus() ? (
              <div>
                <p
                  style={{
                    margin: "0.25rem 0",
                    "font-weight": networkStatus()?.connected ? "normal" : "bold",
                  }}
                >
                  <strong>Mode:</strong>{" "}
                  {networkStatus()?.mode === "client"
                    ? "WiFi Client"
                    : networkStatus()?.mode === "ap"
                      ? "Access Point"
                      : "Disconnected"}
                </p>
                {networkStatus()?.connected && (
                  <>
                    <p style={{ margin: "0.25rem 0" }}>
                      <strong>Network:</strong> {networkStatus()?.ssid}
                    </p>
                    <p style={{ margin: "0.25rem 0" }}>
                      <strong>IP Address:</strong> {networkStatus()?.ip}
                    </p>
                    {networkStatus()?.rssi !== undefined && (
                      <p style={{ margin: "0.25rem 0" }}>
                        <strong>Signal Strength:</strong> {networkStatus()?.rssi} dBm
                      </p>
                    )}
                    {networkStatus()?.clients !== undefined && (
                      <p style={{ margin: "0.25rem 0" }}>
                        <strong>Connected Clients:</strong> {networkStatus()?.clients}
                      </p>
                    )}
                  </>
                )}
              </div>
            ) : (
              <p style={{ margin: "0", "font-style": "italic", color: "#666" }}>
                Loading network status...
              </p>
            )}
          </div>
        )}

        {isEditing() ? (
          <div>
            <div style={{ "margin-bottom": "1rem" }}>
              <label style={{ display: "block", "margin-bottom": "0.5rem" }}>
                <strong>SSID:</strong>
              </label>
              {availableSSIDs().length > 0 && (
                <div
                  style={{
                    "margin-bottom": "0.5rem",
                    display: "flex",
                    gap: "0.5rem",
                    "align-items": "center",
                  }}
                >
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
                      flex: 1,
                      padding: "0.5rem",
                      "border-radius": "4px",
                      border: "1px solid #ccc",
                      "font-size": "0.9rem",
                      "background-color": "#f8f9fa",
                    }}
                  >
                    <option value="">Select a network to use...</option>
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
                  <button
                    onClick={loadAvailableSSIDs}
                    disabled={loadingSSIDs()}
                    style={{
                      padding: "0.25rem 0.5rem",
                      "font-size": "0.8rem",
                      border: "1px solid #ccc",
                      "border-radius": "3px",
                      background: "#f8f9fa",
                      cursor: loadingSSIDs() ? "not-allowed" : "pointer",
                      "white-space": "nowrap",
                    }}
                    title="Refresh available networks"
                  >
                    {loadingSSIDs() ? "⟳" : "🔄"}
                  </button>
                </div>
              )}
              {loadingSSIDs() && availableSSIDs().length === 0 ? (
                <div
                  style={{
                    width: "100%",
                    padding: "0.5rem",
                    "border-radius": "4px",
                    border: "1px solid #ccc",
                    "background-color": "#f8f9fa",
                    "text-align": "center",
                    color: "#666",
                    "margin-bottom": "0.5rem",
                  }}
                >
                  🔄 Scanning for networks...
                </div>
              ) : null}
              <input
                type="text"
                value={editSsid()}
                onInput={(e) => setEditSsid(e.currentTarget.value)}
                placeholder="Enter WiFi network name"
                autocomplete="off"
                style={{
                  width: "100%",
                  padding: "0.5rem",
                  "border-radius": "4px",
                  border: "1px solid #ccc",
                  "font-size": "1rem",
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
                placeholder="Enter WiFi password"
                autocomplete="new-password"
                style={{
                  width: "100%",
                  padding: "0.5rem",
                  "border-radius": "4px",
                  border: "1px solid #ccc",
                  "font-size": "1rem",
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
            <p>
              <strong>SSID:</strong> {networkInfo()?.ssid}
            </p>
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
            <button onClick={handleConfigure}>Configure</button>
            <button onClick={props.onClose} style={{ "margin-left": "0.5rem" }}>
              Close
            </button>
          </>
        )}
      </PopupFooter>
    </Popup>
  );
};
