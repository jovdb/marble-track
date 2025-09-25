import { Device } from "./Device";
import { debounce } from "@solid-primitives/scheduled";
import { createSignal } from "solid-js";
import deviceStyles from "./Device.module.css";
import pwmMotorStyles from "./PwmMotor.module.css";
import { createPwmMotorStore } from "../../stores/PwmMotor";
import { StepperIcon } from "../icons/Icons"; // Using stepper icon as closest to motor
import PwmMotorConfig from "./PwmMotorConfig";

export function PwmMotor(props: { id: string }) {
  const { state, error, setDutyCycle } = createPwmMotorStore(props.id);
  const [currentDutyCycle, setCurrentDutyCycle] = createSignal(0);
  const [animationDuration, setAnimationDuration] = createSignal(0); // Duration in seconds

  const debouncedSetDutyCycle = debounce((value: number) => {
    const durationMs = animationDuration() * 1000; // Convert to milliseconds
    setDutyCycle(value, durationMs > 0 ? durationMs : undefined);
  }, 100);

  return (
    <Device
      id={props.id}
      deviceState={state()}
      configComponent={<PwmMotorConfig id={props.id} />}
      icon={<StepperIcon />}
    >
      {error() && <div class={deviceStyles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={deviceStyles.device__status}>
            <div
              classList={{
                [pwmMotorStyles["pwm-motor__status-indicator"]]: true,
                [pwmMotorStyles["pwm-motor__status-indicator--moving"]]: Boolean(
                  state()?.running
                ),
                [pwmMotorStyles["pwm-motor__status-indicator--off"]]: !state()?.running,
              }}
            ></div>
            <span class={deviceStyles["device__status-text"]}>
              {state()?.running
                ? `Running at ${state()?.dutyCycle?.toFixed(1) || 0}%`
                : `Stopped (${state()?.dutyCycle?.toFixed(1) || 0}%)`}
            </span>
          </div>

          <div class={deviceStyles["device__input-group"]}>
            <label class={deviceStyles.device__label} for={`dutyCycle-${props.id}`}>
              Duty Cycle: {(currentDutyCycle() || state()?.dutyCycle || 0).toFixed(0)}%
            </label>
            <input
              id={`dutyCycle-${props.id}`}
              class={deviceStyles.device__input}
              type="range"
              min="0"
              max="100"
              step="1"
              value={currentDutyCycle() || state()?.dutyCycle || 0}
              onInput={(e) => {
                const value = Number(e.currentTarget.value);
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
              onInput={(e) => {
                const value = Number(e.currentTarget.value);
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
                setDutyCycle(0, durationMs > 0 ? durationMs : undefined);
              }}
            >
              0%
            </button>
            <button
              class={deviceStyles.device__button}
              onClick={() => {
                const durationMs = animationDuration() * 1000;
                setCurrentDutyCycle(25);
                setDutyCycle(25, durationMs > 0 ? durationMs : undefined);
              }}
            >
              25%
            </button>
            <button
              class={deviceStyles.device__button}
              onClick={() => {
                const durationMs = animationDuration() * 1000;
                setCurrentDutyCycle(50);
                setDutyCycle(50, durationMs > 0 ? durationMs : undefined);
              }}
            >
              50%
            </button>
            <button
              class={deviceStyles.device__button}
              onClick={() => {
                const durationMs = animationDuration() * 1000;
                setCurrentDutyCycle(75);
                setDutyCycle(75, durationMs > 0 ? durationMs : undefined);
              }}
            >
              75%
            </button>
            <button
              class={deviceStyles.device__button}
              onClick={() => {
                const durationMs = animationDuration() * 1000;
                setCurrentDutyCycle(100);
                setDutyCycle(100, durationMs > 0 ? durationMs : undefined);
              }}
            >
              100%
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
