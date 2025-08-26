import styles from "./Device.module.css";
import { IWsDeviceMessage, sendMessage } from "../../hooks/useWebSocket";
import { createWheelStore, IWheelConfig } from "../../stores/Wheel";
import { createMemo, For, onMount } from "solid-js";

// Update the import path below to the correct location of IWheelState

export function WheelConfig(props: { id: string }) {
  const { state, error, calibrate, getChildStateByType, config, saveConfig, loadConfig } =
    createWheelStore(props.id);
  const currentPosition = createMemo(() => getChildStateByType("stepper")()?.currentPosition);

  onMount(() => {
    loadConfig();
  });

  const moveSteps = (steps: number) => {
    const stepper = state()?.children?.find((c) => c.type === "stepper");
    if (!stepper) return;
    sendMessage({
      type: "device-fn",
      deviceType: "stepper",
      deviceId: stepper.id,
      fn: "move",
      args: {
        steps,
      },
    } as IWsDeviceMessage);
  };

  return (
    <div class={styles.device__controls}>
      <button
        class={styles.device__button}
        onClick={() => {
          calibrate();
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
      <div>
        Breakpoints:
        <ul>
          <For each={config()?.breakPoints}>
            {(bp, index) => (
              <li style={{ display: "flex", "align-items": "center", gap: "0.5em" }}>
                {bp}
                <button
                  onClick={() => {
                    const cfg = config() || ({} as IWheelConfig);
                    cfg.breakPoints = cfg.breakPoints.slice() || [];
                    cfg.breakPoints.splice(index(), 1);
                    saveConfig(cfg);
                  }}
                >
                  X
                </button>
                <button
                  disabled={index() === 0}
                  title="Move Up"
                  onClick={() => {
                    const cfg = config() || ({} as IWheelConfig);
                    const arr = cfg.breakPoints.slice() || [];
                    if (index() > 0) {
                      [arr[index() - 1], arr[index()]] = [arr[index()], arr[index() - 1]];
                      cfg.breakPoints = arr;
                      saveConfig(cfg);
                    }
                  }}
                >↑</button>
                <button
                  disabled={index() === (config()?.breakPoints.length - 1)}
                  title="Move Down"
                  onClick={() => {
                    const cfg = config() || ({} as IWheelConfig);
                    const arr = cfg.breakPoints.slice() || [];
                    if (index() < arr.length - 1) {
                      [arr[index() + 1], arr[index()]] = [arr[index()], arr[index() + 1]];
                      cfg.breakPoints = arr;
                      saveConfig(cfg);
                    }
                  }}
                >↓</button>
              </li>
            )}
          </For>
          <button
            onClick={() => {
              const cfg = config() || ({} as IWheelConfig);
              cfg.breakPoints = cfg.breakPoints.slice() || [];
              cfg.breakPoints.push(Math.floor(Math.random() * 1000));
              saveConfig(cfg);
            }}
          >
            +
          </button>
        </ul>
      </div>
      <div>
        <button
          class={`${styles["device__button"]}]}`}
          onClick={() => {
            saveConfig(config()?.breakPoints);
          }}
          disabled={!!error()}
        >
          Save
        </button>
      </div>
    </div>
  );
}
