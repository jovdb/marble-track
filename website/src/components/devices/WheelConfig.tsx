import styles from "./Device.module.css";
import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { IWheelState } from "./Wheel";
import { createMemo } from "solid-js";
import { IStepperState } from "./Stepper";

// Update the import path below to the correct location of IWheelState

export function WheelConfig(props: { id: string }) {
  const [deviceState, connectedState, disabled] = createDeviceState<IWheelState>(props.id);

  const [stepperState] = createDeviceState<IWheelState>(props.id + "-stepper");

  const onCalibrateClicked = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "calibrate",
      })
    );
  };

  const onUnblock = () => {
    const stepper = deviceState()?.children?.find((c) => c.type === "STEPPER");
    if (!stepper) return;
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: stepper.id,
        fn: "move",
        steps: -100,
      })
    );
  };

  const moveSteps = (steps: number) => {
    const stepper = deviceState()?.children?.find((c) => c.type === "STEPPER");
    if (!stepper) return;
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: stepper.id,
        fn: "move",
        steps,
      })
    );
  };

  return (
    <div class={styles.device__controls}>
      <button class={styles.device__button} onClick={onCalibrateClicked} disabled={disabled()}>
        Calibrate
      </button>
      <button class={styles.device__button} onClick={onUnblock} disabled={disabled()}>
        Unblock
      </button>
      Current: {stepperState()?.currentPosition ?? "?"}
      <div style={{ display: "flex", gap: "0.5em", "margin-top": "1em" }}>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(-200)}
          disabled={disabled()}
        >
          -200
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(-50)}
          disabled={disabled()}
        >
          -50
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(-10)}
          disabled={disabled()}
        >
          -10
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(10)}
          disabled={disabled()}
        >
          +10
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(50)}
          disabled={disabled()}
        >
          +50
        </button>
        <button
          class={`${styles["device__button"]} ${styles["device__button-small"]}`}
          onClick={() => moveSteps(200)}
          disabled={disabled()}
        >
          +200
        </button>
      </div>
    </div>
  );
}
