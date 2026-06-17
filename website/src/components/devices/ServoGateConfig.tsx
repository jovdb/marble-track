import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useServoGate } from "../../stores/ServoGate";

interface ServoGateConfigProps {
  id: string;
  onClose: () => void;
}

export default function ServoGateConfig(props: ServoGateConfigProps) {
  const [device, actions] = useServoGate(props.id);

  const [name, setName] = createSignal(device()?.config?.name ?? device()?.id ?? "ServoGate");
  const [openDelayMs, setOpenDelayMs] = createSignal(String(device()?.config?.openDelayMs ?? 500));
  const [closeDelayMs, setCloseDelayMs] = createSignal(
    String(device()?.config?.closeDelayMs ?? 1000)
  );
  const [betweenDelayMs, setBetweenDelayMs] = createSignal(
    String(device()?.config?.betweenDelayMs ?? 500)
  );
  const [fullQueueCount, setFullQueueCount] = createSignal(
    String(device()?.config?.fullQueueCount ?? 5)
  );
  const [initialQueueCount, setInitialQueueCount] = createSignal(
    String(device()?.config?.initialQueueCount ?? 0)
  );

  const toNumber = (value: string, fallback = 0) => {
    const num = Number(value);
    return Number.isFinite(num) ? num : fallback;
  };

  createEffect(() => {
    const config = device()?.config;
    if (!config) return;
    if (typeof config.name === "string") setName(config.name);
    if (typeof config.openDelayMs === "number") setOpenDelayMs(String(config.openDelayMs));
    if (typeof config.closeDelayMs === "number") setCloseDelayMs(String(config.closeDelayMs));
    if (typeof config.betweenDelayMs === "number") setBetweenDelayMs(String(config.betweenDelayMs));
    if (typeof config.fullQueueCount === "number") setFullQueueCount(String(config.fullQueueCount));
    if (typeof config.initialQueueCount === "number")
      setInitialQueueCount(String(config.initialQueueCount));
  });

  return (
    <DeviceConfig
      device={device}
      onSave={() =>
        actions.setDeviceConfig({
          name: name()?.trim() || device()?.id,
          openDelayMs: toNumber(openDelayMs(), 500),
          closeDelayMs: toNumber(closeDelayMs(), 1000),
          betweenDelayMs: toNumber(betweenDelayMs(), 500),
          fullQueueCount: toNumber(fullQueueCount(), 5),
          initialQueueCount: toNumber(initialQueueCount(), 0),
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
          <DeviceConfigItem name="Open delay (ms):">
            <input
              type="number"
              value={openDelayMs()}
              min={0}
              step={50}
              onInput={(e) => setOpenDelayMs(e.currentTarget.value)}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="Delay between button press and servo moving to 100%"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Close delay (ms):">
            <input
              type="number"
              value={closeDelayMs()}
              min={0}
              step={50}
              onInput={(e) => setCloseDelayMs(e.currentTarget.value)}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="Hold time at 100% before servo returns to 0%"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Between delay (ms):">
            <input
              type="number"
              value={betweenDelayMs()}
              min={0}
              step={50}
              onInput={(e) => setBetweenDelayMs(e.currentTarget.value)}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="Pause between consecutive queued cycles"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Full queue count:">
            <input
              type="number"
              value={fullQueueCount()}
              min={1}
              step={1}
              onInput={(e) => setFullQueueCount(e.currentTarget.value)}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="Queue size at which the button is considered held (queue fills to this value)"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Initial queue count:">
            <input
              type="number"
              value={initialQueueCount()}
              min={0}
              step={1}
              onInput={(e) => setInitialQueueCount(e.currentTarget.value)}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="Initial value for the queue count (default 0)"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
