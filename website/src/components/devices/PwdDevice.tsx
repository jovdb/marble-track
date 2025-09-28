import { Device } from "./Device";
import { debounce } from "@solid-primitives/scheduled";
import { createEffect, createMemo, createSignal } from "solid-js";
import deviceStyles from "./Device.module.css";
import pwdDeviceStyles from "./PwdDevice.module.css";
import { usePwdDevice } from "../../stores/PwdDevice";
import { StepperIcon } from "../icons/Icons";
import PwdDeviceConfig from "./PwdDeviceConfig";

export function PwdDevice(props: { id: string }) {
  const pwdDeviceStore = usePwdDevice(props.id);
  const device = () => pwdDeviceStore[0];
  const actions = pwdDeviceStore[1];

  const deviceState = createMemo(() => device()?.state);
  const [currentDutyCycle, setCurrentDutyCycle] = createSignal<number | undefined>(undefined);
  const [animationDuration, setAnimationDuration] = createSignal(0);

  createEffect(() => {
    const duty = deviceState()?.dutyCycle;
    if (duty !== undefined) {
      setCurrentDutyCycle(duty);
    }
  });

  const debouncedSetDutyCycle = debounce((value: number) => {
    const durationMs = animationDuration() * 1000;
    actions.setDutyCycle(value, durationMs > 0 ? durationMs : undefined);
  }, 100);

  const sliderValue = createMemo(() => currentDutyCycle() ?? deviceState()?.dutyCycle ?? 0);
  const statusText = createMemo(() => {
    const duty = deviceState()?.dutyCycle ?? 0;
    return deviceState()?.running ? `Running at ${duty.toFixed(1)}%` : `Stopped (${duty.toFixed(1)}%)`;
  });

  return (
    <Device
      id={props.id}
      deviceState={deviceState()}
      configComponent={(onClose) => <PwdDeviceConfig id={props.id} onClose={onClose} />}
      icon={<StepperIcon />}
    >
      <div class={deviceStyles.device__status}>
        <div
          classList={{
            [pwdDeviceStyles["pwd-device__status-indicator"]]: true,
            [pwdDeviceStyles["pwd-device__status-indicator--moving"]]: Boolean(deviceState()?.running),
            [pwdDeviceStyles["pwd-device__status-indicator--off"]]: !deviceState()?.running,
          }}
        ></div>
        <span class={deviceStyles["device__status-text"]}>{statusText()}</span>
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`pwd-dutyCycle-${props.id}`}>
          Duty Cycle: {sliderValue().toFixed(0)}%
        </label>
        <input
          id={`pwd-dutyCycle-${props.id}`}
          class={deviceStyles.device__input}
          type="range"
          min="0"
          max="100"
          step="1"
          value={sliderValue()}
          onInput={(event) => {
            const value = Number(event.currentTarget.value);
            setCurrentDutyCycle(value);
            debouncedSetDutyCycle(value);
          }}
        />
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`pwd-duration-${props.id}`}>
          Animation Duration: {animationDuration().toFixed(1)}s
          {animationDuration() === 0 && " (Instant)"}
        </label>
        <input
          id={`pwd-duration-${props.id}`}
          class={deviceStyles.device__input}
          type="range"
          min="0"
          max="10"
          step="0.1"
          value={animationDuration()}
          onInput={(event) => {
            const value = Number(event.currentTarget.value);
            setAnimationDuration(value);
          }}
        />
      </div>

      <div class={deviceStyles.device__controls}>
        <button
          class={deviceStyles.device__button}
          onClick={() => {
            const durationMs = animationDuration() * 1000;
            setCurrentDutyCycle(0);
            actions.setDutyCycle(0, durationMs > 0 ? durationMs : undefined);
          }}
        >
          0%
        </button>
        <button
          class={deviceStyles.device__button}
          onClick={() => {
            const durationMs = animationDuration() * 1000;
            setCurrentDutyCycle(25);
            actions.setDutyCycle(25, durationMs > 0 ? durationMs : undefined);
          }}
        >
          25%
        </button>
        <button
          class={deviceStyles.device__button}
          onClick={() => {
            const durationMs = animationDuration() * 1000;
            setCurrentDutyCycle(50);
            actions.setDutyCycle(50, durationMs > 0 ? durationMs : undefined);
          }}
        >
          50%
        </button>
        <button
          class={deviceStyles.device__button}
          onClick={() => {
            const durationMs = animationDuration() * 1000;
            setCurrentDutyCycle(75);
            actions.setDutyCycle(75, durationMs > 0 ? durationMs : undefined);
          }}
        >
          75%
        </button>
        <button
          class={deviceStyles.device__button}
          onClick={() => {
            const durationMs = animationDuration() * 1000;
            setCurrentDutyCycle(100);
            actions.setDutyCycle(100, durationMs > 0 ? durationMs : undefined);
          }}
        >
          100%
        </button>
      </div>
    </Device>
  );
}
