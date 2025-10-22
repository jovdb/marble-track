import styles from "./Device.module.css";
import wheelStyles from "./WheelConfig.module.css";
import { useWheel, IWheelConfig } from "../../stores/Wheel";
import { createMemo, For, onMount, createSignal } from "solid-js";
import DeviceConfig from "./DeviceConfig";
import { useDevice } from "../../stores/Devices";

export function WheelConfig(props: { id: string; onClose: () => void }) {
  const wheelStore = useWheel(props.id);
  const device = () => wheelStore[0];
  const actions = wheelStore[1];

  const config = () => device()?.config;
  // TODO: Handle error state - might need to be added to device state
  const error = () => undefined; // Placeholder until error handling is implemented

  const [stepperDevice, { sendMessage }] = useDevice(`${props.id}-stepper`);
  const currentPosition = createMemo(() => (stepperDevice?.state as any)?.currentPosition);
  const [deviceName, setDeviceName] = createSignal("");

  onMount(() => {
    actions.getDeviceConfig();
  });

  const moveSteps = (steps: number) => {
    // For now, assume the stepper device ID is wheelId + "-stepper"
    const stepperDeviceId = `${props.id}-stepper`;

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
        name: deviceName() || currentConfig.name || props.id,
      };
      actions.setDeviceConfig(updatedConfig);
    }
  };

  // Update device name when config loads
  const currentConfig = createMemo(() => config());
  createMemo(() => {
    const cfg = currentConfig();
    if (cfg && typeof cfg.name === "string" && !deviceName()) {
      setDeviceName(cfg.name);
    }
  });

  return (
    <DeviceConfig id={props.id} onSave={handleSave} onClose={props.onClose}>
      <div style={{ "margin-bottom": "1em" }}>
        <label style={{ display: "block", "margin-bottom": "0.5em" }}>Device Name:</label>
        <input
          type="text"
          value={deviceName()}
          onInput={(e) => setDeviceName(e.currentTarget.value)}
          style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
        />
      </div>
      <button
        class={styles.device__button}
        onClick={() => {
          actions.calibrate();
        }}
      >
        Calibrate
      </button>
      Current: {currentPosition() ?? "?"}
      <div style={{ display: "flex", gap: "0.5em", "margin-top": "1em" }}>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(-200)}
          disabled={!!error()}
        >
          -200
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(-50)}
          disabled={!!error()}
        >
          -50
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(-10)}
          disabled={!!error()}
        >
          -10
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(10)}
          disabled={!!error()}
        >
          +10
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(50)}
          disabled={!!error()}
        >
          +50
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(200)}
          disabled={!!error()}
        >
          +200
        </button>
      </div>
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
