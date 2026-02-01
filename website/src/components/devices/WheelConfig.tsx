import styles from "./Device.module.css";
import wheelStyles from "./WheelConfig.module.css";
import { createMemo, For, onMount, createSignal, onCleanup } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useWheel } from "../../stores/Wheel";
import { WheelGraphic } from "./WheelGraphic";
import { useWebSocket2 } from "../../hooks/useWebSocket";

export function WheelConfig(props: { device: any; actions: any; onClose: () => void }) {
  const device = () => props.device;
  const actions = props.actions;

  const config = () => device()?.config;

  const [deviceName, setDeviceName] = createSignal("");
  const [isNameDirty, setIsNameDirty] = createSignal(false);
  const [stepsPerRevolution, setStepsPerRevolution] = createSignal("");
  const [isStepsDirty, setIsStepsDirty] = createSignal(false);
  const [maxStepsPerRevolution, setMaxStepsPerRevolution] = createSignal("");
  const [isMaxStepsDirty, setIsMaxStepsDirty] = createSignal(false);
  const [angle, setAngle] = createSignal("");
  const [breakpoints, setBreakpoints] = createSignal<number[]>([]);
  const [zeroPointDegree, setZeroPointDegree] = createSignal("");
  const [isZeroPointDirty, setIsZeroPointDirty] = createSignal(false);

  const toNumber = (value: string, fallback = 0) => {
    const num = Number(value);
    return Number.isFinite(num) ? num : fallback;
  };

  const [, { subscribe }] = useWebSocket2();
  const [, wheelActions] = useWheel(device()?.id);

  onMount(() => {
    actions.getDeviceConfig();

    // Subscribe to WebSocket messages for steps-per-revolution updates
    const unsubscribe = subscribe((message: any) => {
      if (message.type === "steps-per-revolution" && message.deviceId === device()?.id) {
        const oldValue = toNumber(stepsPerRevolution());
        const newValue = message.steps;
        setStepsPerRevolution(String(newValue));
        // Set max steps per revolution with 5% extra of the received value
        const maxValue = Math.round(newValue * 1.05);
        setMaxStepsPerRevolution(String(maxValue));
        console.log(`Steps per revolution updated from ${oldValue} to ${newValue}, max set to ${maxValue}`);
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
        stepsPerRevolution: toNumber(stepsPerRevolution()),
        maxStepsPerRevolution: toNumber(maxStepsPerRevolution()),
        breakPoints: breakpoints(),
        zeroPointDegree: toNumber(zeroPointDegree()),
      };
      actions.setDeviceConfig(updatedConfig);
    }
  };

  // Update device name, steps per revolution, max steps per revolution, and breakpoints when config loads
  const currentConfig = createMemo(() => config());
  createMemo(() => {
    const cfg = currentConfig();
    if (cfg && typeof cfg.name === "string" && !isNameDirty()) {
      setDeviceName(cfg.name);
    }
    if (cfg && typeof cfg.stepsPerRevolution === "number" && !isStepsDirty()) {
      setStepsPerRevolution(String(cfg.stepsPerRevolution));
    }
    if (cfg && typeof cfg.maxStepsPerRevolution === "number" && !isMaxStepsDirty()) {
      setMaxStepsPerRevolution(String(cfg.maxStepsPerRevolution));
    }
    if (cfg && cfg.breakPoints && breakpoints().length === 0) {
      setBreakpoints([...cfg.breakPoints]);
    }
    if (cfg && typeof cfg.zeroPointDegree === "number" && !isZeroPointDirty()) {
      setZeroPointDegree(String(cfg.zeroPointDegree));
    }
  });

  return (
    <DeviceConfig device={device()} onSave={handleSave} onClose={props.onClose}>
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name:">
            <input
              type="text"
              value={deviceName()}
              onInput={(e) => {
                setIsNameDirty(true);
                setDeviceName(e.currentTarget.value);
              }}
              style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Steps per Revolution:">
            <input
              type="number"
              value={stepsPerRevolution()}
              onInput={(e) => {
                setIsStepsDirty(true);
                setStepsPerRevolution(e.currentTarget.value);
              }}
              style={{ flex: "1", padding: "0.5em", "font-size": "1em" }}
              min="0"
            />
            <button
              class={styles.device__button}
              onClick={(e) => {
                e.preventDefault(); // prevent post
                
                if (device()?.state?.state === "CALIBRATING") {
                  wheelActions.stop();
                  console.log("Stopping calibration");
                } else {
                  wheelActions.calibrate(toNumber(maxStepsPerRevolution()));
                }
              }}
              style={{ "flex-shrink": "0" }}
            >
              {device()?.state?.state === "CALIBRATING" ? "Stop" : "Calibrate"}
            </button>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Max Steps per Revolution:">
            <input
              title="Used for calibration to known when to stop searching for zero switch."
              type="number"
              value={maxStepsPerRevolution()}
              onInput={(e) => {
                setIsMaxStepsDirty(true);
                setMaxStepsPerRevolution(e.currentTarget.value);
              }}
              style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
              min="1"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem>
            <WheelGraphic
              angle={toNumber(angle())}
              breakpoints={breakpoints()}
              zeroPointDegree={toNumber(zeroPointDegree())}
              isCalibrated={!!device()?.state?.lastZeroPosition}
              isSearchingZero={
                device()?.state?.state === "CALIBRATING" && !device()?.state?.lastZeroPosition
              }
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Zero Point Degree:">
            <input
              type="number"
              value={zeroPointDegree()}
              onInput={(e) => {
                setIsZeroPointDirty(true);
                setZeroPointDegree(e.currentTarget.value);
              }}
              style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
              min="0"
              max="359.9"
              step="0.1"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Angle (0-359.9°):">
            <input
              type="number"
              value={angle()}
              onInput={(e) => {
                const value = e.currentTarget.value;
                if (value === "") {
                  setAngle("");
                  return;
                }
                const num = parseFloat(value);
                if (!isNaN(num) && num >= 0 && num <= 359.9) {
                  setAngle(String(Math.round(num * 10) / 10));
                }
              }}
              onKeyPress={(e) => {
                if (e.key === "Enter") {
                  e.preventDefault();
                  // Trigger move to button if enabled
                  const isMoving = device()?.state?.state === "MOVING";
                  const isDisabled =
                    !isMoving &&
                    (!toNumber(stepsPerRevolution()) ||
                    toNumber(stepsPerRevolution()) <= 0 ||
                    device()?.state?.lastZeroPosition === 0);
                  
                  if (!isDisabled) {
                    if (isMoving) {
                      wheelActions.stop();
                      console.log("Stopping wheel");
                    } else {
                      const targetAngle = toNumber(angle());
                      wheelActions.moveToAngle(targetAngle);
                      console.log(`Moving to angle ${targetAngle}°`);
                    }
                  }
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
                
                if (device()?.state?.state === "INIT") {
                  wheelActions.stop();
                  console.log("Stopping init");
                } else {
                  wheelActions.init(toNumber(maxStepsPerRevolution()));
                }
              }}
              style={{ "flex-shrink": "0" }}
              disabled={device()?.state?.state === "RESET"}
            >
              {device()?.state?.state === "INIT" ? "Stop" : "Init"}
            </button>
            <button
              class={styles.device__button}
              onClick={(e) => {
                e.preventDefault(); // prevent post
                
                if (device()?.state?.state === "MOVING") {
                  wheelActions.stop();
                  console.log("Stopping wheel");
                } else {
                  const targetAngle = toNumber(angle());
                  wheelActions.moveToAngle(targetAngle);
                  console.log(`Moving to angle ${targetAngle}°`);
                }
              }}
              style={{ "flex-shrink": "0" }}
              disabled={
                device()?.state?.state !== "MOVING" &&
                (!toNumber(stepsPerRevolution()) ||
                toNumber(stepsPerRevolution()) <= 0 ||
                device()?.state?.lastZeroPosition === 0)
              }
            >
              {device()?.state?.state === "MOVING" ? "Stop" : "Move to"}
            </button>
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>

      <div
        style={{ display: "flex", gap: "1rem", "align-items": "flex-start", "flex-wrap": "wrap" }}
      >
        <button
          onClick={(e) => {
            e.preventDefault(); // prevent post
            const currentBreakpoints = breakpoints();
            const newBreakpoint = toNumber(angle());
            // Find the insertion index to maintain increasing order
            const insertIndex = currentBreakpoints.findIndex(bp => bp >= newBreakpoint);
            const newBreakpoints = [...currentBreakpoints];
            if (insertIndex === -1) {
              // Append if larger than all existing
              newBreakpoints.push(newBreakpoint);
            } else {
              // Insert at the found index
              newBreakpoints.splice(insertIndex, 0, newBreakpoint);
            }
            setBreakpoints(newBreakpoints);
          }}
        >
          + Add Breakpoint
        </button>
        <div style={{ flex: "1 1 320px", "min-width": "280px" }}>
          <div class={wheelStyles["wheel-config__breakpoints"]}>
            <label class={wheelStyles["wheel-config__label"]}>Breakpoints:</label>
            <ul class={wheelStyles["wheel-config__list"]}>
              <For each={breakpoints()}>
                {(bp, index) => (
                  <li class={wheelStyles["wheel-config__item"]}>
                    <span class={wheelStyles["wheel-config__value"]}>{bp}</span>

                    <button
                      disabled={index() === 0}
                      title="Move Up"
                      onClick={(e) => {
                        e.preventDefault();
                        const currentBreakpoints = breakpoints();
                        const newBreakpoints = [...currentBreakpoints];
                        if (index() > 0) {
                          [newBreakpoints[index() - 1], newBreakpoints[index()]] = [
                            newBreakpoints[index()],
                            newBreakpoints[index() - 1],
                          ];
                          setBreakpoints(newBreakpoints);
                        }
                      }}
                    >
                      ↑
                    </button>
                    <button
                      disabled={index() === breakpoints().length - 1}
                      title="Move Down"
                      onClick={(e) => {
                        e.preventDefault();
                        const currentBreakpoints = breakpoints();
                        const newBreakpoints = [...currentBreakpoints];
                        if (index() < newBreakpoints.length - 1) {
                          [newBreakpoints[index() + 1], newBreakpoints[index()]] = [
                            newBreakpoints[index()],
                            newBreakpoints[index() + 1],
                          ];
                          setBreakpoints(newBreakpoints);
                        }
                      }}
                    >
                      ↓
                    </button>
                    <button
                      title="Delete"
                      onClick={(e) => {
                        e.preventDefault();
                        const currentBreakpoints = breakpoints();
                        const newBreakpoints = currentBreakpoints.filter((_, i) => i !== index());
                        setBreakpoints(newBreakpoints);
                      }}
                    >
                      ✕
                    </button>
                  </li>
                )}
              </For>
            </ul>
          </div>
        </div>
      </div>
    </DeviceConfig>
  );
}
