import { Device } from "./Device";
import { createMemo } from "solid-js";
import styles from "./Device.module.css";
import "./Wheel.module.css";

import { WheelConfig } from "./WheelConfig";
import { WheelGraphic } from "./WheelGraphic";
import { IWheelState, useWheel } from "../../stores/Wheel";
import { useWheelAnimation } from "../../hooks/useWheelAnimation";

export function Wheel(props: { id: string }) {
  const wheelStore = useWheel(props.id);
  const device = () => wheelStore[0];
  const actions = wheelStore[1];

  const state = () => device()?.state;
  // TODO: Handle error state - might need to be added to device state
  const error = () => undefined; // Placeholder until error handling is implemented

  const uiAngle = useWheelAnimation(state);

  const onNextClicked = () => {
    actions.nextBreakpoint();
  };

  const isCalibrated = createMemo(() => !!state()?.lastZeroPosition);
  const isSearchingZero = createMemo(() => !state()?.lastZeroPosition && state()?.state !== "IDLE");

  // Radius handled internally by WheelGraphic now

  // Get breakpoints from device config
  const breakpoints = () => device()?.config?.breakPoints || [];
  const zeroPointDegree = () => device()?.config?.zeroPointDegree || 0;

  function getStateString(state: IWheelState["state"] | undefined) {
    switch (state) {
      case "IDLE":
        return "Idle";
      case "CALIBRATING":
        return "Calibrating...";
      case "RESET":
        return "Resetting...";
      case "MOVING":
        return "Moving...";
      case undefined:
        return "";
      default:
        return "Unknown";
    }
  }

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => (
        <WheelConfig device={device()} actions={actions} onClose={onClose} />
      )}
    >
      <div style={{ "max-width": "300px", margin: "0 auto" }}>
        <WheelGraphic
          angle={uiAngle() ?? 0}
          size={100}
          breakpoints={breakpoints()}
          zeroPointDegree={zeroPointDegree()}
          isCalibrated={isCalibrated()}
          isSearchingZero={isSearchingZero()}
        />
      </div>
      {error() && <div class={styles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={styles.device__status}>
            <div>
              <span class={styles["device__status-text"]}>
                Status: {getStateString(state()?.state)}
              </span>
            </div>
            {(state()?.currentBreakpointIndex ?? -1) >= 0 && (
              <div>
                <span>[{(state()?.currentBreakpointIndex ?? 0) + 1}]</span>
              </div>
            )}
            {state()?.angle !== null && state()?.angle !== undefined && (
              <div>
                <span class={styles["device__status-text"]}>{state()?.angle?.toFixed(1)}°</span>
              </div>
            )}
            {(state()?.targetBreakpointIndex ?? -1) >= 0 && (
              <div>
                -&gt;
                <span>[{(state()?.targetBreakpointIndex ?? 0) + 1}]</span>
              </div>
            )}
            {state()?.targetAngle !== undefined &&
              state()?.targetAngle !== null &&
              state()?.targetAngle !== state()?.angle && (
                <div>
                  <span class={styles["device__status-text"]}>
                    {state()?.targetAngle?.toFixed(1)}°
                  </span>
                </div>
              )}
          </div>
          <div class={styles.device__controls}>
            <button class={styles.device__button} onClick={() => actions.reset()}>
              Reset
            </button>
            {(() => {
              const nextBreakpointIndex = (() => {
                const target = state()?.targetBreakpointIndex;
                if (target !== undefined && target >= 0) {
                  return target;
                }
                const current = state()?.currentBreakpointIndex ?? -1;
                return (current + 1) % breakpoints().length;
              })();
              const nextBreakpointDisplay = nextBreakpointIndex + 1;
              return (
                <button
                  class={styles.device__button}
                  onClick={onNextClicked}
                  disabled={state()?.lastZeroPosition === 0 || state()?.state === "MOVING"}
                >
                  Next {nextBreakpointDisplay}/{breakpoints().length}
                </button>
              );
            })()}
          </div>
        </>
      )}
    </Device>
  );
}
