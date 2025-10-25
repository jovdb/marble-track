import styles from "./Device.module.css";
import wheelStyles from "./WheelConfig.module.css";
import { IWheelConfig } from "../../stores/Wheel";
import { createMemo, For, onMount, createSignal, onCleanup } from "solid-js";
import DeviceConfig from "./DeviceConfig";
import { useDevice } from "../../stores/Devices";
import { useWebSocket2 } from "../../hooks/useWebSocket2";

export function WheelConfig(props: { device: any; actions: any; onClose: () => void }) {
  const device = () => props.device;
  const actions = props.actions;

  const config = () => device()?.config;

  const [stepperDevice, { sendMessage }] = useDevice(`${device()?.id}-stepper`);
  const currentPosition = createMemo(() => (stepperDevice?.state as any)?.currentPosition);
  const [deviceName, setDeviceName] = createSignal("");
  const [stepsPerRevolution, setStepsPerRevolution] = createSignal(0);

  const [, { subscribe }] = useWebSocket2();

  onMount(() => {
    actions.getDeviceConfig();

    // Subscribe to WebSocket messages for steps-per-revolution updates
    const unsubscribe = subscribe((message: any) => {
      if (message.type === "steps-per-revolution" && message.deviceId === device()?.id) {
        const oldValue = stepsPerRevolution();
        const newValue = message.steps;
        setStepsPerRevolution(newValue);
        console.log(`Steps per revolution updated from ${oldValue} to ${newValue}`);
      }
    });

    onCleanup(() => {
      unsubscribe();
    });
  });

  const moveSteps = (steps: number) => {
    // For now, assume the stepper device ID is wheelId + "-stepper"
    const stepperDeviceId = `${device()?.id}-stepper`;

    sendMessage({
      type: "device-fn",
      deviceType: "stepper",
      deviceId: stepperDeviceId,
      fn: "move",
      args: {
        steps,
      },
    });
  };

  const handleSave = () => {
    const currentConfig = config();
    if (currentConfig) {
      const updatedConfig = {
        ...currentConfig,
        name: deviceName() || currentConfig.name || device()?.id,
        stepsPerRevolution: stepsPerRevolution(),
      };
      actions.setDeviceConfig(updatedConfig);
    }
  };

  // Update device name and steps per revolution when config loads
  const currentConfig = createMemo(() => config());
  createMemo(() => {
    const cfg = currentConfig();
    if (cfg && typeof cfg.name === "string" && !deviceName()) {
      setDeviceName(cfg.name);
    }
    if (cfg && typeof cfg.stepsPerRevolution === "number" && stepsPerRevolution() === 0) {
      setStepsPerRevolution(cfg.stepsPerRevolution);
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
        <label style={{ display: "block", "margin-bottom": "0.5em" }}>Steps per Revolution:</label>
        <div style={{ display: "flex", gap: "0.5em", "align-items": "flex-end" }}>
          <input
            type="number"
            value={stepsPerRevolution()}
            onInput={(e) => setStepsPerRevolution(parseInt(e.currentTarget.value) || 0)}
            style={{ flex: "1", padding: "0.5em", "font-size": "1em" }}
            min="0"
          />
          <button
            class={styles.device__button}
            onClick={(e) => {
              e.preventDefault(); // prevent post
              actions.calibrate();
            }}
            style={{ "flex-shrink": "0" }}
            disabled={device()?.state?.state === "CALIBRATING"}
          >
            Calibrate
          </button>
        </div>
      </div>
      <button
        class={styles.device__button}
        onClick={(e) => {
          e.preventDefault(); // prevent post
          actions.reset();
        }}
      >
        Reset
      </button>
      Current: {currentPosition() ?? "?"}
      <div class={wheelStyles["wheel-config__breakpoints"]}>
        <label class={wheelStyles["wheel-config__label"]}>Breakpoints:</label>
        <ul class={wheelStyles["wheel-config__list"]}>
          <For each={config()?.breakPoints}>
            {(bp, index) => (
              <li class={wheelStyles["wheel-config__item"]}>
                <span class={wheelStyles["wheel-config__value"]}>{bp}</span>

                <button
                  disabled={index() === 0}
                  title="Move Up"
                  onClick={() => {
                    const cfg = config() || ({} as IWheelConfig);
                    const arr = cfg.breakPoints?.slice() || [];
                    if (index() > 0) {
                      [arr[index() - 1], arr[index()]] = [arr[index()], arr[index() - 1]];
                      cfg.breakPoints = arr;
                      actions.setDeviceConfig(cfg);
                    }
                  }}
                >
                  ↑
                </button>
                <button
                  disabled={index() === (config()?.breakPoints?.length ?? 0) - 1}
                  title="Move Down"
                  onClick={() => {
                    const cfg = config() || ({} as IWheelConfig);
                    const arr = cfg.breakPoints?.slice() || [];
                    if (index() < arr.length - 1) {
                      [arr[index() + 1], arr[index()]] = [arr[index()], arr[index() + 1]];
                      cfg.breakPoints = arr;
                      actions.setDeviceConfig(cfg);
                    }
                  }}
                >
                  ↓
                </button>
                <button
                  title="Delete"
                  onClick={() => {
                    const cfg = config() || ({} as IWheelConfig);
                    cfg.breakPoints = cfg.breakPoints?.slice() || [];
                    cfg.breakPoints.splice(index(), 1);
                    actions.setDeviceConfig(cfg);
                  }}
                >
                  ✕
                </button>
              </li>
            )}
          </For>
        </ul>
        <button
          onClick={() => {
            const cfg = config() || ({} as IWheelConfig);
            cfg.breakPoints = cfg.breakPoints?.slice() || [];
            cfg.breakPoints.push(Math.floor(Math.random() * 1000));
            actions.setDeviceConfig(cfg);
          }}
        >
          + Add Breakpoint
        </button>
      </div>
    </DeviceConfig>
  );
}
