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
  const [stepsPerRevolution, setStepsPerRevolution] = createSignal(0);
  const [maxStepsPerRevolution, setMaxStepsPerRevolution] = createSignal(10000);
  const [angle, setAngle] = createSignal(0);
  const [breakpoints, setBreakpoints] = createSignal<number[]>([]);
  const [zeroPointDegree, setZeroPointDegree] = createSignal(0);

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
        // Set max steps per revolution with 5% extra of the received value
        const maxValue = Math.round(newValue * 1.05);
        setMaxStepsPerRevolution(maxValue);
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
        stepsPerRevolution: stepsPerRevolution(),
        maxStepsPerRevolution: maxStepsPerRevolution(),
        breakPoints: breakpoints(),
        zeroPointDegree: zeroPointDegree(),
      };
      actions.setDeviceConfig(updatedConfig);
    }
  };

  // Update device name, steps per revolution, max steps per revolution, and breakpoints when config loads
  const currentConfig = createMemo(() => config());
  createMemo(() => {
    const cfg = currentConfig();
    if (cfg && typeof cfg.name === "string" && !deviceName()) {
      setDeviceName(cfg.name);
    }
    if (cfg && typeof cfg.stepsPerRevolution === "number" && stepsPerRevolution() === 0) {
      setStepsPerRevolution(cfg.stepsPerRevolution);
    }
    if (cfg && typeof cfg.maxStepsPerRevolution === "number" && maxStepsPerRevolution() === 10000) {
      setMaxStepsPerRevolution(cfg.maxStepsPerRevolution);
    }
    if (cfg && cfg.breakPoints && breakpoints().length === 0) {
      setBreakpoints([...cfg.breakPoints]);
    }
    if (cfg && typeof cfg.zeroPointDegree === "number") {
      setZeroPointDegree(cfg.zeroPointDegree);
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
              onInput={(e) => setDeviceName(e.currentTarget.value)}
              style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Steps per Revolution:">
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
                
                if (device()?.state?.state === "CALIBRATING") {
                  wheelActions.stop();
                  console.log("Stopping calibration");
                } else {
                  actions.calibrate();
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
              onInput={(e) => setMaxStepsPerRevolution(parseInt(e.currentTarget.value) || 10000)}
              style={{ width: "100%", padding: "0.5em", "font-size": "1em" }}
              min="1"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem>
            <WheelGraphic
              angle={angle()}
              breakpoints={breakpoints()}
              zeroPointDegree={zeroPointDegree()}
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
              onInput={(e) => setZeroPointDegree(parseFloat(e.currentTarget.value) || 0)}
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
                
                if (device()?.state?.state === "INIT") {
                  wheelActions.stop();
                  console.log("Stopping init");
                } else {
                  wheelActions.init();
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
                  const targetAngle = angle();
                  wheelActions.moveToAngle(targetAngle);
                  console.log(`Moving to angle ${targetAngle}°`);
                }
              }}
              style={{ "flex-shrink": "0" }}
              disabled={
                device()?.state?.state !== "MOVING" &&
                (!stepsPerRevolution() ||
                stepsPerRevolution() <= 0 ||
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
            setBreakpoints([...currentBreakpoints, angle()]);
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
