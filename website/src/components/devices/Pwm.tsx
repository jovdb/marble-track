import { createMemo, createSignal, onCleanup } from "solid-js";
import { Device } from "./Device";
import styles from "./Device.module.css";
import pwmStyles from "./Pwm.module.css";
import PwmConfig from "./PwmConfig";
import { getDeviceIcon } from "../icons/Icons";
import { usePwm } from "../../stores/Pwm";

export function Pwm(props: { id: string }) {
  const pwmStore = usePwm(props.id);
  const device = () => pwmStore[0];
  const actions = pwmStore[1];

  const deviceType = device()?.type;
  const dutyCycle = createMemo(() => device()?.state?.dutyCycle ?? 0);
  const frequency = createMemo(() => device()?.state?.frequency ?? 5000);
  const resolution = createMemo(() => device()?.state?.resolution ?? 8);
  const maxDutyCycle = createMemo(() => Math.pow(2, resolution()) - 1);
  const dutyCyclePercent = createMemo(() => Math.round((dutyCycle() / maxDutyCycle()) * 100));

  const statusLabel = createMemo(
    () => `Duty Cycle: ${dutyCyclePercent()}% (${dutyCycle()}/${maxDutyCycle()})`
  );

  // Debounced slider value
  const [sliderValue, setSliderValue] = createSignal(dutyCycle());
  let debounceTimeout: number | undefined;

  // Update slider value when device state changes
  createMemo(() => {
    setSliderValue(dutyCycle());
  });

  const handleDutyCycleChange = (event: Event) => {
    const target = event.target as HTMLInputElement;
    const value = parseInt(target.value, 10);
    if (!isNaN(value)) {
      setSliderValue(value);

      // Clear existing timeout
      if (debounceTimeout) {
        clearTimeout(debounceTimeout);
      }

      // Set new timeout to send update after 500ms of no changes
      debounceTimeout = setTimeout(() => {
        actions.setDutyCycle(value);
        debounceTimeout = undefined;
      }, 500) as unknown as number;
    }
  };

  // Cleanup timeout on component unmount
  onCleanup(() => {
    if (debounceTimeout) {
      clearTimeout(debounceTimeout);
    }
  });

  const sliderPercent = createMemo(() => Math.round((sliderValue() / maxDutyCycle()) * 100));

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <PwmConfig id={props.id} onClose={onClose} />}
      icon={deviceType ? getDeviceIcon(deviceType) : null}
    >
      <div class={styles.device__status}>
        <div
          class={pwmStyles["pwm__status-indicator"]}
          style={{
            "--duty-cycle-percent": `${dutyCyclePercent()}%`,
          }}
          role="status"
          aria-label={statusLabel()}
        ></div>
        <span class={styles["device__status-text"]}>{statusLabel()}</span>
      </div>
      <div class={styles.device__controls}>
        <div class={pwmStyles["pwm__slider-container"]}>
          <label for={`pwm-${props.id}-duty-cycle`} class={pwmStyles["pwm__slider-label"]}>
            Duty Cycle:
          </label>
          <input
            id={`pwm-${props.id}-duty-cycle`}
            type="range"
            min="0"
            max={maxDutyCycle()}
            value={sliderValue()}
            onInput={handleDutyCycleChange}
            class={pwmStyles["pwm__slider"]}
          />
          <span class={pwmStyles["pwm__slider-value"]}>{sliderPercent()}%</span>
        </div>
        <div class={pwmStyles["pwm__info"]}>
          <span>Frequency: {frequency()} Hz</span>
          <span>Resolution: {resolution()} bits</span>
        </div>
      </div>
    </Device>
  );
}
