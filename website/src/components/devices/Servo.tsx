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
  const statusText = createMemo(() => {
    const value = sliderValue();
    return deviceState()?.running
      ? `Moving from ${deviceState()?.value?.toFixed(0)}% to ${deviceState()?.targetValue?.toFixed(0) ?? value.toFixed(0)}%`
      : `Position: ${value.toFixed(0)}%`;
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
            [servoStyles["servo__status-indicator--moving"]]: Boolean(deviceState()?.running),
            [servoStyles["servo__status-indicator--off"]]: !deviceState()?.running,
          }}
        ></div>
        <span class={deviceStyles["device__status-text"]}>{statusText()}</span>
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
    </Device>
  );
}
