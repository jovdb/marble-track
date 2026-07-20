import { For, createEffect, createMemo, createSignal } from "solid-js";
import { useI2c } from "../../../stores/I2c";

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
  const [i2cDevice, { scanBus }] = useI2c(() => props.i2cDeviceId);
  const [isScanning, setIsScanning] = createSignal(false);
  const [scanError, setScanError] = createSignal("");

  // Re-scan whenever the selected I2C bus changes.
  createEffect(() => {
    const busId = props.i2cDeviceId;
    if (busId) {
      setIsScanning(true);
      setScanError("");
      scanBus();
    } else {
      setScanError("Please select an I²C bus first");
    }
  });

  // Reset scanning status when we get new addresses or an error
  createEffect(() => {
    const state = i2cDevice()?.state;
    const error = i2cDevice()?.stateErrorMessage;

    if (state?.foundAddresses || error) {
      setIsScanning(false);
      setScanError(error || "");
    }
  });

  const requestScan = () => {
    if (!props.i2cDeviceId) {
      setScanError("Please select an I²C bus first");
      return;
    }
    setIsScanning(true);
    setScanError("");
    scanBus();
  };

  const addresses = createMemo(() => {
    const scanned = i2cDevice()?.state?.foundAddresses ?? [];
    return scanned.length > 0 ? scanned : (props.defaultAddresses ?? []);
  });

  return (
    <>
      <select
        value={props.value}
        onChange={(e) => props.onChange(Number(e.currentTarget.value))}
        style={{ "margin-left": "0.5rem" }}
      >
        <For each={addresses()}>
          {(addr) => (
            <option value={addr}>0x{addr.toString(16).toUpperCase().padStart(2, "0")}</option>
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
