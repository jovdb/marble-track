import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useLauncher } from "../../stores/Launcher";

interface LauncherConfigProps {
  id: string;
  onClose: () => void;
}

export default function LauncherConfig(props: LauncherConfigProps) {
  const [device, actions] = useLauncher(props.id);

  const [name, setName] = createSignal(device()?.config?.name ?? device()?.id ?? "Launcher");
  const [loadTimeMs, setLoadTimeMs] = createSignal(String(device()?.config?.loadTimeMs ?? 2000));
  const [launchTimeMs, setLaunchTimeMs] = createSignal(String(device()?.config?.launchTimeMs ?? 0));

  const toNumber = (value: string, fallback = 0) => {
    const num = Number(value);
    return Number.isFinite(num) ? num : fallback;
  };

  createEffect(() => {
    const config = device()?.config;
    if (!config) return;
    if (typeof config.name === "string") setName(config.name);
    if (typeof config.loadTimeMs === "number") setLoadTimeMs(String(config.loadTimeMs));
    if (typeof config.launchTimeMs === "number") setLaunchTimeMs(String(config.launchTimeMs));
  });

  return (
    <DeviceConfig
      device={device()}
      onSave={() =>
        actions.setDeviceConfig({
          name: name()?.trim() || device()?.id,
          loadTimeMs: toNumber(loadTimeMs(), 2000),
          launchTimeMs: toNumber(launchTimeMs(), 0),
        })
      }
      onClose={props.onClose}
    >
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
          <DeviceConfigItem name="Load time (ms):">
            <input
              type="number"
              value={loadTimeMs()}
              min={0}
              step={100}
              onInput={(e) => setLoadTimeMs(e.currentTarget.value)}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="Duration of slow arm movement used for loading (ms)"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Launch time (ms):">
            <input
              type="number"
              value={launchTimeMs()}
              min={0}
              step={50}
              onInput={(e) => setLaunchTimeMs(e.currentTarget.value)}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="Duration of fast arm movement used for launching (0 = instant)"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
