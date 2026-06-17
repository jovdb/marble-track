import { For, createEffect, createSignal, createMemo } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useDevices } from "../../stores/Devices";
import { useBattery } from "../../stores/Battery";

interface BatteryConfigProps {
  id: string;
  onClose: () => void;
}

export default function BatteryConfig(props: BatteryConfigProps) {
  const [device, { setDeviceConfig }] = useBattery(props.id);
  const [devicesStore] = useDevices();
  const devicesState = () => devicesStore; // Wrap in a function to avoid stale closure issues

  const cfg = () => device?.config;

  const [name, setName] = createSignal(cfg()?.name ?? "Battery");
  const [powerMonitorDeviceId, setPowerMonitorDeviceId] = createSignal(
    cfg()?.powerMonitorDeviceId ?? ""
  );
  const [minVoltage, setMinVoltage] = createSignal(cfg()?.minVoltage ?? 15.0);
  const [maxVoltage, setMaxVoltage] = createSignal(cfg()?.maxVoltage ?? 21.0);

  // All PowerMonitor devices to choose from
  const powerMonitorDevices = createMemo(() =>
    Object.values(devicesState().devices).filter((d) => d.type === "powermonitor")
  );

  createEffect(() => {
    const c = device?.config;
    if (!c) return;
    if (typeof c.name === "string") setName(c.name);
    if (typeof c.powerMonitorDeviceId === "string") setPowerMonitorDeviceId(c.powerMonitorDeviceId);
    if (typeof c.minVoltage === "number") setMinVoltage(c.minVoltage);
    if (typeof c.maxVoltage === "number") setMaxVoltage(c.maxVoltage);
  });

  const handleSave = () => {
    setDeviceConfig({
      name: name(),
      powerMonitorDeviceId: powerMonitorDeviceId(),
      minVoltage: minVoltage(),
      maxVoltage: maxVoltage(),
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
          <DeviceConfigItem name="Power Monitor:">
            <select
              value={powerMonitorDeviceId()}
              onChange={(e) => setPowerMonitorDeviceId(e.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            >
              <option value="">Select Power Monitor…</option>
              <For each={powerMonitorDevices()}>
                {(pm) => (
                  <option value={pm.id}>{(pm.config as { name?: string })?.name || pm.id}</option>
                )}
              </For>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Min Voltage (V):">
            <input
              type="number"
              min={0}
              step={0.1}
              value={minVoltage()}
              onInput={(e) => setMinVoltage(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem", width: "6rem" }}
            />
            <span style={{ "margin-left": "0.5rem", "font-size": "0.8rem", color: "#666" }}>
              0% battery / low-voltage alert
            </span>
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Max Voltage (V):">
            <input
              type="number"
              min={0}
              step={0.1}
              value={maxVoltage()}
              onInput={(e) => setMaxVoltage(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem", width: "6rem" }}
            />
            <span style={{ "margin-left": "0.5rem", "font-size": "0.8rem", color: "#666" }}>
              100% battery reference (Li-ion 5S: 21 V)
            </span>
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
