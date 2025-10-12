import { Device } from "./Device";
import { debounce } from "@solid-primitives/scheduled";
import { createEffect, createMemo, createSignal } from "solid-js";
import deviceStyles from "./Device.module.css";
import pwmStyles from "./Pwm.module.css";
import { usePwm } from "../../stores/Pwm";
import { StepperIcon } from "../icons/Icons";
import PwmConfig from "./PwmConfig";

export function Pwm(props: { id: string }) {
  const pwmStore = usePwm(props.id);
  const device = () => pwmStore[0];
  const actions = pwmStore[1];

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
    return deviceState()?.running
      ? `Running at ${duty.toFixed(1)}%`
      : `Stopped (${duty.toFixed(1)}%)`;
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <PwmConfig id={props.id} onClose={onClose} />}
      icon={<StepperIcon />}
    >
      <div class={deviceStyles.device__status}>
        <div
          classList={{
            [pwmStyles["pwm__status-indicator"]]: true,
            [pwmStyles["pwm__status-indicator--running"]]: Boolean(deviceState()?.running),
            [pwmStyles["pwm__status-indicator--stopped"]]: !deviceState()?.running,
          }}
        ></div>
        <span class={deviceStyles["device__status-text"]}>{statusText()}</span>
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`pwm-duty-${props.id}`}>
          Duty Cycle: {sliderValue().toFixed(0)}%
        </label>
        <input
          id={`pwm-duty-${props.id}`}
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
        <label class={deviceStyles.device__label} for={`pwm-duration-${props.id}`}>
          Animation Duration: {animationDuration().toFixed(1)}s
          {animationDuration() === 0 && " (Instant)"}
        </label>
        <input
          id={`pwm-duration-${props.id}`}
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
