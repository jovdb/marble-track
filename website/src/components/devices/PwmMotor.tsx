import { Device } from "./Device";
import { debounce } from "@solid-primitives/scheduled";
import { createEffect, createMemo, createSignal } from "solid-js";
import deviceStyles from "./Device.module.css";
import pwmMotorStyles from "./PwmMotor.module.css";
import { usePwmMotor } from "../../stores/PwmMotor";
import { StepperIcon } from "../icons/Icons"; // Using stepper icon as closest to motor
import PwmMotorConfig from "./PwmMotorConfig";

export function PwmMotor(props: { id: string }) {
  const pwmMotorStore = usePwmMotor(props.id);
  const device = () => pwmMotorStore[0];
  const actions = pwmMotorStore[1];

  const deviceState = createMemo(() => device()?.state);
  const [currentDutyCycle, setCurrentDutyCycle] = createSignal<number | undefined>(undefined);
  const [animationDuration, setAnimationDuration] = createSignal(0); // Duration in seconds

  createEffect(() => {
    const duty = deviceState()?.dutyCycle;
    if (duty !== undefined) {
      setCurrentDutyCycle(duty);
    }
  });

  const debouncedSetDutyCycle = debounce((value: number) => {
    const durationMs = animationDuration() * 1000; // Convert to milliseconds
    actions.setDutyCycle(value, durationMs > 0 ? durationMs : undefined);
  }, 100);

  const sliderValue = createMemo(() => currentDutyCycle() ?? deviceState()?.dutyCycle ?? 0);
  const minDutyCycle = createMemo(() => device()?.config?.minDutyCycle ?? 0);
  const maxDutyCycle = createMemo(() => device()?.config?.maxDutyCycle ?? 100);
  const statusText = createMemo(() => {
    const duty = deviceState()?.dutyCycle ?? 0;
    return deviceState()?.running
      ? `Running at ${duty.toFixed(1)}%`
      : `Stopped (${duty.toFixed(1)}%)`;
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <PwmMotorConfig id={props.id} onClose={onClose} />}
      icon={<StepperIcon />}
    >
      <div class={deviceStyles.device__status}>
        <div
          classList={{
            [pwmMotorStyles["pwm-motor__status-indicator"]]: true,
            [pwmMotorStyles["pwm-motor__status-indicator--moving"]]: Boolean(
              deviceState()?.running
            ),
            [pwmMotorStyles["pwm-motor__status-indicator--off"]]: !deviceState()?.running,
          }}
        ></div>
        <span class={deviceStyles["device__status-text"]}>{statusText()}</span>
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`dutyCycle-${props.id}`}>
          Duty Cycle: {sliderValue().toFixed(1)}%
        </label>
        <input
          id={`dutyCycle-${props.id}`}
          class={deviceStyles.device__input}
          type="range"
          min={minDutyCycle()}
          max={maxDutyCycle()}
          step="0.1"
          value={sliderValue()}
          onInput={(event) => {
            const value = Number(event.currentTarget.value);
            setCurrentDutyCycle(value);
            debouncedSetDutyCycle(value);
          }}
        />
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`duration-${props.id}`}>
          Animation Duration: {animationDuration().toFixed(1)}s
          {animationDuration() === 0 && " (Instant)"}
        </label>
        <input
          id={`duration-${props.id}`}
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
    </Device>
  );
}
