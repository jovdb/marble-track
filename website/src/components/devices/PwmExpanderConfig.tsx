import { For, createEffect, createSignal, createMemo } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { IDevice, useDevices } from "../../stores/Devices";
import { usePwmExpander } from "../../stores/PwmExpander";
import { II2cState, II2cConfig } from "../../stores/I2c";
import { I2cAddressPicker } from "./shared/I2cAddressPicker";

interface PwmExpanderConfigProps {
  id: string;
  onClose: () => void;
}

export default function PwmExpanderConfig(props: PwmExpanderConfigProps) {
  const [device, { setDeviceConfig }] = usePwmExpander(props.id);
  const [devicesStore] = useDevices();
  const devicesState = () => devicesStore; // Wrap in a function to avoid stale closure issues

  const [name, setName] = createSignal<string>(device()?.config?.name ?? "PWM Expander");
  const [i2cAddress, setI2cAddress] = createSignal<number>(device()?.config?.i2cAddress ?? 0x40);
  const [i2cDeviceId, setI2cDeviceId] = createSignal<string>(device()?.config?.i2cDeviceId ?? "");
  const [frequency, setFrequency] = createSignal<number>(device()?.config?.frequency ?? 50);

  const i2cDevices = createMemo(
    () =>
      Object.values(devicesState().devices).filter((d) => d.type === "i2c") as IDevice<
        II2cState,
        II2cConfig
      >[]
  );

  createEffect(() => {
    const cfg = device()?.config;
    if (!cfg) return;

    if (typeof cfg.name === "string") setName(cfg.name);
    if (typeof cfg.i2cAddress === "number") setI2cAddress(cfg.i2cAddress);
    if (typeof cfg.i2cDeviceId === "string") setI2cDeviceId(cfg.i2cDeviceId);
    if (typeof cfg.frequency === "number") setFrequency(cfg.frequency);
  });

  const handleSave = () => {
    setDeviceConfig({
      name: name(),
      i2cDeviceId: i2cDeviceId(),
      i2cAddress: i2cAddress(),
      frequency: frequency(),
    });
  };

  return (
    <DeviceConfig device={device()} onSave={handleSave} onClose={props.onClose}>
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
            <I2cAddressPicker
              i2cDeviceId={i2cDeviceId()}
              value={i2cAddress()}
              onChange={setI2cAddress}
              defaultAddresses={Array.from({ length: 64 }, (_, i) => 0x40 + i)}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
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
