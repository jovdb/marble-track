import { For, createEffect, createSignal, createMemo } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useDevices } from "../../stores/Devices";
import { usePowerMonitor } from "../../stores/PowerMonitor";
import { II2cConfig } from "../../stores/I2c";
import { I2cAddressPicker } from "./shared/I2cAddressPicker";

// INA226 supports 16 addresses: 0x40–0x4F
const INA226_DEFAULT_ADDRESSES = Array.from({ length: 16 }, (_, i) => 0x40 + i);

interface PowerMonitorConfigProps {
  id: string;
  onClose: () => void;
}

export default function PowerMonitorConfig(props: PowerMonitorConfigProps) {
  const [device, { setDeviceConfig }] = usePowerMonitor(props.id);
  const [devicesStore] = useDevices();
  const devicesState = () => devicesStore; // Wrap in a function to avoid stale closure issues

  const cfg = () => device()?.config;

  const [name, setName] = createSignal(cfg()?.name ?? "Power Monitor");
  const [i2cDeviceId, setI2cDeviceId] = createSignal(cfg()?.i2cDeviceId ?? "");
  const [i2cAddress, setI2cAddress] = createSignal(cfg()?.i2cAddress ?? 0x40);
  const [shuntResistance, setShuntResistance] = createSignal(cfg()?.shuntResistance ?? 0.1);
  const [maxCurrent, setMaxCurrent] = createSignal(cfg()?.maxCurrent ?? 3.2);

  const i2cDevices = createMemo(() =>
    Object.values(devicesState().devices).filter((d) => d.type === "i2c")
  );

  // Sync signals when device config arrives
  createEffect(() => {
    const c = device()?.config;
    if (!c) return;
    if (typeof c.name === "string") setName(c.name);
    if (typeof c.i2cDeviceId === "string") setI2cDeviceId(c.i2cDeviceId);
    if (typeof c.i2cAddress === "number") setI2cAddress(c.i2cAddress);
    if (typeof c.shuntResistance === "number") setShuntResistance(c.shuntResistance);
    if (typeof c.maxCurrent === "number") setMaxCurrent(c.maxCurrent);
  });

  const handleSave = () => {
    setDeviceConfig({
      name: name(),
      i2cDeviceId: i2cDeviceId(),
      i2cAddress: i2cAddress(),
      shuntResistance: shuntResistance(),
      maxCurrent: maxCurrent(),
    });
  };

  return (
    <DeviceConfig device={device} onSave={handleSave} onClose={props.onClose}>
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name:">
            <input
              type="text"
              value={name()}
              onInput={(e) => setName(e.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="I²C Bus:">
            <select
              value={i2cDeviceId()}
              onChange={(e) => setI2cDeviceId(e.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            >
              <option value="">Select I²C Bus…</option>
              <For each={i2cDevices()}>
                {(i2cDevice) => (
                  <option value={i2cDevice.id}>
                    {(i2cDevice.config as II2cConfig)?.name || i2cDevice.id}
                  </option>
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
              defaultAddresses={INA226_DEFAULT_ADDRESSES}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Shunt Resistance (Ω):">
            <input
              type="number"
              min={0.001}
              max={1}
              step={0.001}
              value={shuntResistance()}
              onInput={(e) => setShuntResistance(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem", width: "6rem" }}
            />
            <span style={{ "margin-left": "0.5rem", "font-size": "0.8rem", color: "#666" }}>
              e.g. 0.1 Ω or 0.01 Ω
            </span>
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Max Current (A):">
            <input
              type="number"
              min={0.1}
              max={100}
              step={0.1}
              value={maxCurrent()}
              onInput={(e) => setMaxCurrent(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem", width: "6rem" }}
            />
            <span style={{ "margin-left": "0.5rem", "font-size": "0.8rem", color: "#666" }}>
              Used for INA226 calibration
            </span>
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
