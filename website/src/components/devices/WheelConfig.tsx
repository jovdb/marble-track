import styles from "./Device.module.css";
import { createDeviceState, IWsDeviceMessage, sendMessage } from "../../hooks/useWebSocket";
import { IWheelState } from "./Wheel";
import { IStepperState } from "../../stores/Stepper";

// Update the import path below to the correct location of IWheelState

export function WheelConfig(props: { id: string }) {
  const [deviceState, , disabled] = createDeviceState<IWheelState>(props.id);

  const [stepperState] = createDeviceState<IStepperState>(props.id + "-stepper");

  const onCalibrateClicked = () => {
    sendMessage({
      type: "device-fn",
      deviceType: "wheel",
      deviceId: props.id,
      fn: "calibrate",
    } as IWsDeviceMessage);
  };

  const onUnblock = () => {
    const stepper = deviceState()?.children?.find((c) => c.type === "STEPPER");
    if (!stepper) return;
    sendMessage({
      type: "device-fn",
      deviceType: "stepper",
      deviceId: stepper.id,
      fn: "move",
      args: {
        steps: -100,
      },
    } as IWsDeviceMessage);
  };

  const moveSteps = (steps: number) => {
    const stepper = deviceState()?.children?.find((c) => c.type === "STEPPER");
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
