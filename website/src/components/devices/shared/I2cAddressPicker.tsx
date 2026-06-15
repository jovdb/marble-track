import { For, createEffect, createSignal } from "solid-js";
import { useWebSocket2 } from "../../../hooks/useWebSocket";
import type { IWsReceiveExpanderAddressesMessage } from "../../../interfaces/WebSockets";

interface I2cAddressPickerProps {
  /** The I2C bus device ID to scan. Re-scans automatically when this changes. */
  i2cDeviceId: string;
  /** Currently selected address value. */
  value: number;
  /** Called when the user selects a different address. */
  onChange: (address: number) => void;
  /** Fallback addresses shown when no scan result is available yet. */
  defaultAddresses?: number[];
}

/**
 * Reusable I2C address picker: shows a dropdown of scanned/default addresses
 * plus a "Scan" button. Used by IoExpanderConfig, PwmExpanderConfig, and
 * PowerMonitorConfig.
 */
export function I2cAddressPicker(props: I2cAddressPickerProps) {
  const [, { sendMessage, subscribe }] = useWebSocket2();
  const [availableAddresses, setAvailableAddresses] = createSignal<number[]>([]);
  const [isScanning, setIsScanning] = createSignal(false);
  const [scanError, setScanError] = createSignal("");

  // Re-subscribe and re-scan whenever the selected I2C bus changes.
  createEffect(() => {
    const busId = props.i2cDeviceId; // reactive dependency
    const unsubscribe = subscribe((msg) => {
      if (msg.type === "expander-addresses") {
        const expanderMsg = msg as IWsReceiveExpanderAddressesMessage;
        setIsScanning(false);
        if ("error" in expanderMsg) {
          setScanError(expanderMsg.error);
          setAvailableAddresses([]);
        } else {
          setScanError("");
          setAvailableAddresses(expanderMsg.addresses);
        }
      }
    });

    if (busId) {
      setIsScanning(true);
      setScanError("");
      sendMessage({ type: "expander-addresses", i2cDeviceId: busId });
    } else {
      setScanError("Please select an I²C bus first");
    }

    return unsubscribe;
  });

  const requestScan = () => {
    const busId = props.i2cDeviceId;
    if (!busId) {
      setScanError("Please select an I²C bus first");
      return;
    }
    setIsScanning(true);
    setScanError("");
    sendMessage({ type: "expander-addresses", i2cDeviceId: busId });
  };

  const addresses = () => {
    const scanned = availableAddresses();
    return scanned.length > 0 ? scanned : (props.defaultAddresses ?? []);
  };

  return (
    <>
      <select
        value={props.value}
        onChange={(e) => props.onChange(Number(e.currentTarget.value))}
        style={{ "margin-left": "0.5rem" }}
      >
        <For each={addresses()}>
          {(addr) => (
            <option value={addr}>
              0x{addr.toString(16).toUpperCase().padStart(2, "0")}
            </option>
          )}
        </For>
      </select>
      <button
        type="button"
        onClick={requestScan}
        disabled={isScanning()}
        style={{ "margin-left": "0.5rem" }}
      >
        {isScanning() ? "Scanning…" : "Scan"}
      </button>
      {scanError() && (
        <span style={{ "margin-left": "0.5rem", color: "red", "font-size": "0.85rem" }}>
          {scanError()}
        </span>
      )}
    </>
  );
}
