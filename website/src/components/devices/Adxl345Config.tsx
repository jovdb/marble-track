import { For, createEffect, createSignal, createMemo } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useDevices } from "../../stores/Devices";
import { useAdxl345 } from "../../stores/Adxl345";
import { I2cAddressPicker } from "./shared/I2cAddressPicker";

interface Adxl345ConfigProps {
  id: string;
  onClose: () => void;
}

export default function Adxl345Config(props: Adxl345ConfigProps) {
  const [device, { setDeviceConfig, calibrate }] = useAdxl345(props.id);
  const [devicesStore] = useDevices();
  const devicesState = () => devicesStore;

  const [name, setName] = createSignal(device()?.config?.name ?? "Accelerometer");
  const [i2cDeviceId, setI2cDeviceId] = createSignal(device()?.config?.i2cDeviceId ?? "");
  const [i2cAddress, setI2cAddress] = createSignal(device()?.config?.i2cAddress ?? 0x53);
  const [range, setRange] = createSignal(device()?.config?.range ?? 16);
  const [refreshIntervalMs, setRefreshIntervalMs] = createSignal(
    device()?.config?.refreshIntervalMs ?? 100
  );
  const [offsetX, setOffsetX] = createSignal(device()?.config?.offsetX ?? 0);
  const [offsetY, setOffsetY] = createSignal(device()?.config?.offsetY ?? 0);
  const [offsetZ, setOffsetZ] = createSignal(device()?.config?.offsetZ ?? 0);

  // All I2C devices to choose from
  const i2cDevices = createMemo(() =>
    Object.values(devicesState().devices).filter((d) => d.type === "i2c")
  );

  createEffect(() => {
    const c = device()?.config;
    if (!c) return;
    if (typeof c.name === "string") setName(c.name);
    if (typeof c.i2cDeviceId === "string") setI2cDeviceId(c.i2cDeviceId);
    if (typeof c.i2cAddress === "number") setI2cAddress(c.i2cAddress);
    if (typeof c.range === "number") setRange(c.range);
    if (typeof c.refreshIntervalMs === "number") setRefreshIntervalMs(c.refreshIntervalMs);
    if (typeof c.offsetX === "number") setOffsetX(c.offsetX);
    if (typeof c.offsetY === "number") setOffsetY(c.offsetY);
    if (typeof c.offsetZ === "number") setOffsetZ(c.offsetZ);
  });

  const handleSave = () => {
    setDeviceConfig({
      name: name(),
      i2cDeviceId: i2cDeviceId(),
      i2cAddress: i2cAddress(),
      range: range(),
      refreshIntervalMs: refreshIntervalMs(),
      offsetX: offsetX(),
      offsetY: offsetY(),
      offsetZ: offsetZ(),
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
          <DeviceConfigItem name="I2C Bus:">
            <select
              value={i2cDeviceId()}
              onChange={(e) => setI2cDeviceId(e.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            >
              <option value="">Select I2C Bus...</option>
              <For each={i2cDevices()}>
                {(i2c) => (
                  <option value={i2c.id}>
                    {(i2c.config as { name?: string })?.name || i2c.id}
                  </option>
                )}
              </For>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="I2C Address:">
            <I2cAddressPicker
              i2cDeviceId={i2cDeviceId()}
              value={i2cAddress()}
              onChange={setI2cAddress}
              defaultAddresses={[0x53, 0x1d]}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Range (G):">
            <select
              value={range()}
              onChange={(e) => setRange(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            >
              <option value={2}>2G</option>
              <option value={4}>4G</option>
              <option value={8}>8G</option>
              <option value={16}>16G</option>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Refresh (ms):">
            <input
              type="number"
              value={refreshIntervalMs()}
              onInput={(e) => setRefreshIntervalMs(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem", width: "5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Offsets (X/Y/Z):">
            <div style={{ display: "flex", gap: "0.2rem", "margin-left": "0.5rem" }}>
              <input
                type="number"
                step="0.01"
                value={offsetX()}
                onInput={(e) => setOffsetX(parseFloat(e.currentTarget.value))}
                style={{ width: "3.5rem" }}
              />
              <input
                type="number"
                step="0.01"
                value={offsetY()}
                onInput={(e) => setOffsetY(parseFloat(e.currentTarget.value))}
                style={{ width: "3.5rem" }}
              />
              <input
                type="number"
                step="0.01"
                value={offsetZ()}
                onInput={(e) => setOffsetZ(parseFloat(e.currentTarget.value))}
                style={{ width: "3.5rem" }}
              />
            </div>
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Calibration:">
            <button
              onClick={() => calibrate()}
              style={{ "margin-left": "0.5rem", padding: "0.2rem 0.5rem", "font-size": "0.8rem" }}
            >
              Calibrate Level
            </button>
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
