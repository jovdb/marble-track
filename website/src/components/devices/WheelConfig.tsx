import styles from "./Device.module.css";
import wheelStyles from "./WheelConfig.module.css";
import { sendMessage } from "../../hooks/useWebSocket";
import { createWheelStore, IWheelConfig } from "../../stores/Wheel";
import { createMemo, For, onMount } from "solid-js";
import DeviceConfig from "./DeviceConfig";
import { IWsSendMessage } from "../../interfaces/WebSockets";

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
    } as IWsSendMessage);
  };

  return (
    <DeviceConfig id={props.id} onSave={() => alert(`TODO: save`)}>
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
      <div class={wheelStyles["wheel-config__breakpoints"]}>
        <label class={wheelStyles["wheel-config__label"]}>Breakpoints:</label>
        <ul class={wheelStyles["wheel-config__list"]}>
          <For each={config()?.breakPoints}>
            {(bp, index) => (
              <li class={wheelStyles["wheel-config__item"]}>
                <span class={wheelStyles["wheel-config__value"]}>{bp}</span>

                <button
                  class={wheelStyles["wheel-config__button"]}
                  disabled={index() === 0}
                  title="Move Up"
                  onClick={() => {
                    const cfg = config() || ({} as IWheelConfig);
                    const arr = cfg.breakPoints?.slice() || [];
                    if (index() > 0) {
                      [arr[index() - 1], arr[index()]] = [arr[index()], arr[index() - 1]];
                      cfg.breakPoints = arr;
                      saveConfig(cfg);
                    }
                  }}
                >
                  ↑
                </button>
                <button
                  class={wheelStyles["wheel-config__button"]}
                  disabled={index() === (config()?.breakPoints?.length ?? 0) - 1}
                  title="Move Down"
                  onClick={() => {
                    const cfg = config() || ({} as IWheelConfig);
                    const arr = cfg.breakPoints?.slice() || [];
                    if (index() < arr.length - 1) {
                      [arr[index() + 1], arr[index()]] = [arr[index()], arr[index() + 1]];
                      cfg.breakPoints = arr;
                      saveConfig(cfg);
                    }
                  }}
                >
                  ↓
                </button>
                <button
                  class={`${wheelStyles["wheel-config__button"]} ${wheelStyles["wheel-config__button--delete"]}`}
                  title="Delete"
                  onClick={() => {
                    const cfg = config() || ({} as IWheelConfig);
                    cfg.breakPoints = cfg.breakPoints?.slice() || [];
                    cfg.breakPoints.splice(index(), 1);
                    saveConfig(cfg);
                  }}
                >
                  ✕
                </button>
              </li>
            )}
          </For>
        </ul>
        <button
          class={`${wheelStyles["wheel-config__button"]} ${wheelStyles["wheel-config__button--add"]}`}
          onClick={() => {
            const cfg = config() || ({} as IWheelConfig);
            cfg.breakPoints = cfg.breakPoints?.slice() || [];
            cfg.breakPoints.push(Math.floor(Math.random() * 1000));
            saveConfig(cfg);
          }}
        >
          + Add Breakpoint
        </button>
      </div>
    </DeviceConfig>
  );
}
