import { For, createEffect, createSignal, createMemo } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { IDevice, useDevices } from "../../stores/Devices";
import { useWebSocket2 } from "../../hooks/useWebSocket";
import type { IWsReceiveExpanderAddressesMessage } from "../../interfaces/WebSockets";
import { usePwmExpander } from "../../stores/PwmExpander";
import { II2cState, II2cConfig } from "../../stores/I2c";

interface PwmExpanderConfigProps {
  id: string;
  onClose: () => void;
}

export default function PwmExpanderConfig(props: PwmExpanderConfigProps) {
  const [device] = usePwmExpander(props.id);
  const [devicesStore] = useDevices();
  const [, { sendMessage, subscribe }] = useWebSocket2();

  const [name, setName] = createSignal<string>(device?.config?.name ?? "PWM Expander");
  const [i2cAddress, setI2cAddress] = createSignal<number>(device?.config?.i2cAddress ?? 0x40);
  const [i2cDeviceId, setI2cDeviceId] = createSignal<string>(device?.config?.i2cDeviceId ?? "");
  const [frequency, setFrequency] = createSignal<number>(device?.config?.frequency ?? 50);

  const [availableAddresses, setAvailableAddresses] = createSignal<number[]>([]);
  const [isScanning, setIsScanning] = createSignal<boolean>(false);
  const [scanError, setScanError] = createSignal<string>("");

  const i2cDevices = createMemo(
    () =>
      Object.values(devicesStore.devices).filter((d) => d.type === "i2c") as IDevice<
        II2cState,
        II2cConfig
      >[]
  );

  createEffect(() => {
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

    requestAddresses();
    return unsubscribe;
  });

  const requestAddresses = () => {
    const busId = i2cDeviceId();
    if (!busId) {
      setScanError("Please select an I²C bus first");
      return;
    }
    setIsScanning(true);
    setScanError("");
    sendMessage({ type: "expander-addresses", i2cDeviceId: busId });
  };

  createEffect(() => {
    const cfg = device?.config;
    if (!cfg) return;

    if (typeof cfg.name === "string") setName(cfg.name);
    if (typeof cfg.i2cAddress === "number") setI2cAddress(cfg.i2cAddress);
    if (typeof cfg.i2cDeviceId === "string") setI2cDeviceId(cfg.i2cDeviceId);
    if (typeof cfg.frequency === "number") setFrequency(cfg.frequency);
  });

  const handleSave = () => {
    sendMessage({
      type: "device-save-config",
      deviceId: props.id,
      config: {
        name: name(),
        i2cDeviceId: i2cDeviceId(),
        i2cAddress: i2cAddress(),
        frequency: frequency(),
      },
    });
  };

  // Default PCA9685 addresses (0x40-0x7F)
  const defaultAddresses = Array.from({ length: 64 }, (_, i) => 0x40 + i);

  return (
    <DeviceConfig device={device} onSave={handleSave} onClose={props.onClose}>
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name">
            <input type="text" value={name()} onInput={(e) => setName(e.currentTarget.value)} />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="I²C Bus:">
            <select
              value={i2cDeviceId()}
              onChange={(e) => setI2cDeviceId(e.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            >
              <option value="">Select I²C Bus...</option>
              <For each={i2cDevices()}>
                {(i2cDevice) => (
                  <option value={i2cDevice.id}>{i2cDevice.config?.name || i2cDevice.id}</option>
                )}
              </For>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="I²C Address:">
            <select
              value={i2cAddress()}
              onChange={(e) => setI2cAddress(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            >
              <For each={availableAddresses().length > 0 ? availableAddresses() : defaultAddresses}>
                {(addr) => (
                  <option value={addr}>0x{addr.toString(16).toUpperCase().padStart(2, "0")}</option>
                )}
              </For>
            </select>
            <button
              type="button"
              onClick={requestAddresses}
              disabled={isScanning()}
              style={{ "margin-left": "0.5rem" }}
            >
              {isScanning() ? "Scanning..." : "Scan"}
            </button>
          </DeviceConfigItem>
        </DeviceConfigRow>
        {scanError() && (
          <DeviceConfigRow>
            <DeviceConfigItem name="">
              <div style={{ color: "red", "margin-left": "0.5rem" }}>{scanError()}</div>
            </DeviceConfigItem>
          </DeviceConfigRow>
        )}
        <DeviceConfigRow>
          <DeviceConfigItem name="Frequency (Hz):">
            <input
              type="number"
              min={24}
              max={1526}
              value={frequency()}
              onInput={(e) => setFrequency(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem", width: "6rem" }}
            />
            <span style={{ "margin-left": "0.5rem", "font-size": "0.8rem", color: "#666" }}>
              24–1526 Hz (50 = servo, 1000 = LED)
            </span>
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
