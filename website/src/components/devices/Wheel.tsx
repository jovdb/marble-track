import { Device } from "./Device";
import { createMemo } from "solid-js";
import styles from "./Device.module.css";
import "./Wheel.module.css";

import { WheelConfig } from "./WheelConfig";
import { WheelGraphic } from "./WheelGraphic";
import { useWheel } from "../../stores/Wheel";
import { useWheelAnimation } from "../../hooks/useWheelAnimation";
import { getDeviceIcon } from "../icons/Icons";

export function Wheel(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const wheelStore = useWheel(props.id);
  const device = () => wheelStore[0];
  const actions = wheelStore[1];

  const state = () => device()?.state;
  // TODO: Handle error state - might need to be added to device state
  const error = () => undefined; // Placeholder until error handling is implemented

  const deviceType = device()?.type;
  const uiAngle = useWheelAnimation(state);

  const onNextClicked = () => {
    actions.nextBreakpoint();
  };

  const isCalibrated = createMemo(() => !!state()?.lastZeroPosition);
  const isSearchingZero = createMemo(
    () => !state()?.lastZeroPosition && state()?.state && state()?.state !== "IDLE"
  );

  // Radius handled internally by WheelGraphic now

  // Get breakpoints from device config
  const breakpoints = () => device()?.config?.breakPoints || [];
  const zeroPointDegree = () => device()?.config?.zeroPointDegree || 0;

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => (
        <WheelConfig device={device()} actions={actions} onClose={onClose} />
      )}
      icon={deviceType ? getDeviceIcon(deviceType) : null}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
      stateComponent={() => null}
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
            {state()?.state === "UNKNOWN" && "Unknown"}
            {state()?.state === "CALIBRATING" && "Calibrating..."}
            {state()?.state === "INIT" && "Initializing..."}
            {state()?.state === "IDLE" && "Idle"}
            {state()?.state === "MOVING" &&
              `Moving from breakpoint ${(state()?.currentBreakpointIndex ?? 0) + 1} to ${(state()?.targetBreakpointIndex ?? 0) + 1}${(state()?.targetAngle ?? 0) >= 0 ? ` (${state()?.targetAngle?.toFixed(1)}°)` : ""}...`}
            {state()?.state === "ERROR" && (
              <span style={{ color: "red" }}>
                Error {state()?.errorCode}: {state()?.errorMessage}
              </span>
            )}
          </div>
          <div class={styles.device__controls}>
            <button class={styles.device__button} onClick={() => actions.init()}>
              Find breakpoint 1
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
                  disabled={
                    state()?.state === "MOVING" ||
                    state()?.state === "CALIBRATING" ||
                    state()?.state === "INIT"
                  }
                >
                  Go to next breakpoint {nextBreakpointDisplay}/{breakpoints().length}
                </button>
              );
            })()}
            <button
              class={styles.device__button}
              onClick={() => actions.stop()}
              disabled={state()?.state === "UNKNOWN" || state()?.state === "IDLE"}
            >
              Stop
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
