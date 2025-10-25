import styles from "./Device.module.css";
import wheelStyles from "./WheelConfig.module.css";
import { IWheelConfig } from "../../stores/Wheel";
import { createMemo, For, onMount, createSignal, onCleanup } from "solid-js";
import DeviceConfig from "./DeviceConfig";
import { useDevice } from "../../stores/Devices";
import { useWheel } from "../../stores/Wheel";
import { useWebSocket2 } from "../../hooks/useWebSocket2";

export function WheelConfig(props: { device: any; actions: any; onClose: () => void }) {
  const device = () => props.device;
  const actions = props.actions;

  const config = () => device()?.config;

  const [stepperDevice] = useDevice(`${device()?.id}-stepper`);
  const currentPosition = createMemo(() => (stepperDevice?.state as any)?.currentPosition);
  const [deviceName, setDeviceName] = createSignal("");
  const [stepsPerRevolution, setStepsPerRevolution] = createSignal(0);
  const [angle, setAngle] = createSignal(0);

  const [, { subscribe }] = useWebSocket2();
  const [, wheelActions] = useWheel(device()?.id);

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
      <div style={{ "margin-bottom": "1em" }}>
        <label style={{ display: "block", "margin-bottom": "0.5em" }}>Angle (0-359.9°):</label>
        <div style={{ display: "flex", gap: "0.5em", "align-items": "flex-end" }}>
          <input
            type="number"
            value={angle()}
            onInput={(e) => {
              const value = parseFloat(e.currentTarget.value);
              if (!isNaN(value) && value >= 0 && value <= 359.9) {
                setAngle(Math.round(value * 10) / 10); // Round to 1 decimal place
              }
            }}
            style={{ flex: "1", padding: "0.5em", "font-size": "1em" }}
            min="0"
            max="359.9"
            step="0.1"
          />
          <button
            class={styles.device__button}
            onClick={(e) => {
              e.preventDefault(); // prevent post
              wheelActions.reset();
            }}
            style={{ "flex-shrink": "0" }}
            disabled={device()?.state?.state === "RESET"}
          >
            Reset
          </button>
          <button
            class={styles.device__button}
            onClick={(e) => {
              e.preventDefault(); // prevent post
              const targetAngle = angle();

              wheelActions.moveToAngle(targetAngle);
              console.log(`Moving to angle ${targetAngle}°`);
            }}
            style={{ "flex-shrink": "0" }}
            disabled={!stepsPerRevolution() || stepsPerRevolution() <= 0 || device()?.state?.lastZeroPosition === 0}
          >
            Move to
          </button>
        </div>
      </div>{" "}
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
