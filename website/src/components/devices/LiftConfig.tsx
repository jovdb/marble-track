import { createMemo, onMount, createSignal } from "solid-js";
import DeviceConfig from "./DeviceConfig";

export function LiftConfig(props: { device: any; actions: any; onClose: () => void }) {
  const device = () => props.device;
  const actions = props.actions;

  const config = () => device()?.config;

  const [deviceName, setDeviceName] = createSignal("");
  const [minSteps, setMinSteps] = createSignal(0);
  const [maxSteps, setMaxSteps] = createSignal(1000);

  onMount(() => {
    actions.getDeviceConfig();
  });

  const handleSave = () => {
    const currentConfig = config();
    if (currentConfig) {
      const updatedConfig = {
        ...currentConfig,
        name: deviceName() || currentConfig.name || device()?.id,
        minSteps: minSteps(),
        maxSteps: maxSteps(),
      };
      actions.setDeviceConfig(updatedConfig);
    }
  };

  // Update fields when config loads
  const currentConfig = createMemo(() => config());
  createMemo(() => {
    const cfg = currentConfig();
    if (cfg && typeof cfg.name === "string" && !deviceName()) {
      setDeviceName(cfg.name);
    }
    if (cfg && typeof cfg.minSteps === "number") {
      setMinSteps(cfg.minSteps);
    }
    if (cfg && typeof cfg.maxSteps === "number") {
      setMaxSteps(cfg.maxSteps);
    }
  });

  return (
    <DeviceConfig device={device()} onSave={handleSave} onClose={props.onClose}>
      <div style={{ "margin-bottom": "1em" }}>
        <label style={{ display: "block", "margin-bottom": "0.5em" }}>Name:</label>
        <input
          type="text"
          value={deviceName()}
          onInput={(e) => setDeviceName(e.currentTarget.value)}
          style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
        />
      </div>
      <div style={{ "margin-bottom": "1em" }}>
        <label style={{ display: "block", "margin-bottom": "0.5em" }}>Min Steps:</label>
        <input
          type="number"
          value={minSteps()}
          onInput={(e) => setMinSteps(parseInt(e.currentTarget.value) || 0)}
          style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
          min="0"
        />
      </div>
      <div style={{ "margin-bottom": "1em" }}>
        <label style={{ display: "block", "margin-bottom": "0.5em" }}>Max Steps:</label>
        <input
          type="number"
          value={maxSteps()}
          onInput={(e) => setMaxSteps(parseInt(e.currentTarget.value) || 1000)}
          style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
          min="1"
        />
      </div>
    </DeviceConfig>
  );
}
