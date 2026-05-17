import { Device } from "./Device";
import { debounce } from "@solid-primitives/scheduled";
import { createEffect, createMemo, createSignal } from "solid-js";
import deviceStyles from "./Device.module.css";
import servoStyles from "./Servo.module.css";
import { useServo } from "../../stores/Servo";
import { ServoIcon } from "../icons/Icons";
import ServoConfig from "./ServoConfig";

export function Servo(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const servoStore = useServo(props.id);
  const device = () => servoStore[0];
  const actions = servoStore[1];

  const deviceState = createMemo(() => device()?.state);
  const [currentValue, setCurrentValue] = createSignal<number | undefined>(undefined);
  const [currentDuration, setCurrentDuration] = createSignal<number>(
    device()?.config?.defaultDurationInMs ?? 500
  );

  createEffect(() => {
    const targetValue = deviceState()?.targetValue;
    if (typeof targetValue === "number") {
      setCurrentValue(targetValue);
    } else {
      const value = deviceState()?.value;
      if (typeof value === "number") {
        setCurrentValue(value);
      }
    }
  });

  createEffect(() => {
    const defaultDuration = device()?.config?.defaultDurationInMs;
    if (defaultDuration !== undefined) {
      setCurrentDuration(defaultDuration);
    }
  });

  const debouncedSetValue = debounce((value: number, durationMs: number) => {
    // Convert percentage (0-100%) to normalized value (0.0-1.0)
    const normalizedValue = value / 100;
    actions.setValue(normalizedValue, durationMs);
  }, 100);

  const sliderValue = createMemo(() => currentValue() ?? 0);

  const servoState = createMemo(() => deviceState()?.state ?? "Unknown");
  const isDisabled = createMemo(() => servoState() === "Disabled");
  const isError = createMemo(() => servoState() === "Error");

  const statusText = createMemo(() => {
    const state = servoState();
    if (state === "Moving") {
      return `Moving from ${deviceState()?.value?.toFixed(0)}% to ${deviceState()?.targetValue?.toFixed(0) ?? sliderValue().toFixed(0)}%`;
    }
    if (state === "Disabled") return "Disabled — servo can rotate freely";
    if (state === "Error") return "Error";
    if (state === "Unknown") return "Position unknown";
    return `Position: ${sliderValue().toFixed(0)}%`;
  });

  const indicatorClass = createMemo(() => {
    const state = servoState();
    if (state === "Moving") return servoStyles["servo__status-indicator--moving"];
    if (state === "Ready") return servoStyles["servo__status-indicator--ready"];
    if (state === "Disabled") return servoStyles["servo__status-indicator--disabled"];
    if (state === "Error") return servoStyles["servo__status-indicator--error"];
    return servoStyles["servo__status-indicator--off"];
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <ServoConfig id={props.id} onClose={onClose} />}
      icon={<ServoIcon />}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div class={deviceStyles.device__status}>
        <div
          classList={{
            [servoStyles["servo__status-indicator"]]: true,
            [indicatorClass()]: true,
          }}
        ></div>
        <div class={servoStyles["servo__status-content"]}>
          <span class={deviceStyles["device__status-text"]}>{statusText()}</span>
        </div>
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`value-${props.id}`}>
          Value: {sliderValue().toFixed(0)}%
        </label>
        <input
          id={`value-${props.id}`}
          class={deviceStyles.device__input}
          type="range"
          min="0"
          max="100"
          step="1"
          value={sliderValue()}
          onInput={(event) => {
            const value = Number(event.currentTarget.value);
            setCurrentValue(value);
            debouncedSetValue(value, currentDuration());
          }}
        />
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`duration-${props.id}`}>
          Duration: {currentDuration()}ms
        </label>
        <input
          id={`duration-${props.id}`}
          class={deviceStyles.device__input}
          type="range"
          min="0"
          max="5000"
          step="100"
          value={currentDuration()}
          onInput={(event) => {
            const value = Number(event.currentTarget.value);
            setCurrentDuration(value);
          }}
        />
      </div>

      <div class={deviceStyles["device__controls"]}>
        <button
          class={deviceStyles["device__button"]}
          onClick={() => actions.disable()}
          disabled={isDisabled() || isError() || servoState() === "Unknown"}
          title="Disable PWM signal — servo can rotate freely"
        >
          Disable
        </button>
      </div>
    </Device>
  );
}
