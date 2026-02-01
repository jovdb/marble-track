import { createMemo, onMount, createSignal } from "solid-js";
import DeviceConfig from "./DeviceConfig";

export function LiftConfig(props: { device: any; actions: any; onClose: () => void }) {
  const device = () => props.device;
  const actions = props.actions;

  const config = () => device()?.config;

  const [deviceName, setDeviceName] = createSignal("");
  const [isNameDirty, setIsNameDirty] = createSignal(false);
  const [minSteps, setMinSteps] = createSignal("");
  const [isMinStepsDirty, setIsMinStepsDirty] = createSignal(false);
  const [maxSteps, setMaxSteps] = createSignal("");
  const [isMaxStepsDirty, setIsMaxStepsDirty] = createSignal(false);

  const toNumber = (value: string, fallback = 0) => {
    const num = Number(value);
    return Number.isFinite(num) ? num : fallback;
  };

  onMount(() => {
    actions.getDeviceConfig();
  });

  const handleSave = () => {
    const currentConfig = config();
    if (currentConfig) {
      const updatedConfig = {
        ...currentConfig,
        name: deviceName() || currentConfig.name || device()?.id,
        minSteps: toNumber(minSteps()),
        maxSteps: Math.max(1, toNumber(maxSteps(), 1)),
      };
      actions.setDeviceConfig(updatedConfig);
    }
  };

  // Update fields when config loads
  const currentConfig = createMemo(() => config());
  createMemo(() => {
    const cfg = currentConfig();
    if (cfg && typeof cfg.name === "string" && !isNameDirty()) {
      setDeviceName(cfg.name);
    }
    if (cfg && typeof cfg.minSteps === "number" && !isMinStepsDirty()) {
      setMinSteps(String(cfg.minSteps));
    }
    if (cfg && typeof cfg.maxSteps === "number" && !isMaxStepsDirty()) {
      setMaxSteps(String(cfg.maxSteps));
    }
  });

  return (
    <DeviceConfig device={device()} onSave={handleSave} onClose={props.onClose}>
      <div style={{ "margin-bottom": "1em" }}>
        <label style={{ display: "block", "margin-bottom": "0.5em" }}>Name:</label>
        <input
          type="text"
          value={deviceName()}
            onInput={(e) => {
              setIsNameDirty(true);
              setDeviceName(e.currentTarget.value);
            }}
          style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
        />
      </div>
      <div style={{ "margin-bottom": "1em" }}>
        <label style={{ display: "block", "margin-bottom": "0.5em" }}>Min Steps:</label>
        <input
          type="number"
          value={minSteps()}
            onInput={(e) => {
              setIsMinStepsDirty(true);
              setMinSteps(e.currentTarget.value);
            }}
          style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
          min="0"
        />
      </div>
      <div style={{ "margin-bottom": "1em" }}>
        <label style={{ display: "block", "margin-bottom": "0.5em" }}>Max Steps:</label>
        <input
          type="number"
          value={maxSteps()}
            onInput={(e) => {
              setIsMaxStepsDirty(true);
              setMaxSteps(e.currentTarget.value);
            }}
          style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
          min="1"
        />
      </div>
    </DeviceConfig>
  );
}
