import { createSignal, onMount } from "solid-js";
import DeviceConfig from "./DeviceConfig";
import { createPwmMotorStore } from "../../stores/PwmMotor";

interface PwmMotorConfigProps {
  id: string;
  onClose: () => void;
}

export default function PwmMotorConfig(props: PwmMotorConfigProps) {
  const { state, setupMotor } = createPwmMotorStore(props.id);

  // Configuration signals with default values
  const [pin, setPin] = createSignal(14);
  const [pwmChannel, setPwmChannel] = createSignal(0);
  const [frequency, setFrequency] = createSignal(1000);
  const [resolutionBits, setResolutionBits] = createSignal(10);

  // Load current configuration on mount
  onMount(() => {
    const currentState = state();
    if (currentState) {
      if (currentState.pin !== undefined && currentState.pin !== -1) setPin(currentState.pin);
      if (currentState.pwmChannel !== undefined) setPwmChannel(currentState.pwmChannel);
      if (currentState.frequency !== undefined) setFrequency(currentState.frequency);
      if (currentState.resolutionBits !== undefined) setResolutionBits(currentState.resolutionBits);
    }
  });

  const handleSave = () => {
    setupMotor(pin(), pwmChannel(), frequency(), resolutionBits());
  };

  return (
    <DeviceConfig id={props.id} onSave={handleSave} onClose={props.onClose}>
      <div style={{ display: "grid", "grid-template-columns": "1fr 1fr", gap: "1rem" }}>
        {/* GPIO Pin Configuration */}
        <label style={{ display: "flex", "align-items": "center", gap: "0.5rem" }}>
          GPIO Pin:
          <input
            type="number"
            value={pin()}
            min={0}
            max={39}
            onInput={(e) => setPin(Number(e.currentTarget.value))}
            style={{ width: "4em" }}
            title="GPIO pin number for PWM output (0-39)"
          />
        </label>

        {/* PWM Channel Configuration */}
        <label style={{ display: "flex", "align-items": "center", gap: "0.5rem" }}>
          PWM Channel:
          <select
            value={pwmChannel()}
            onChange={(e) => setPwmChannel(Number(e.currentTarget.value))}
            title="MCPWM channel (0 = MCPWM0A/TIMER0, 1 = MCPWM1A/TIMER1)"
          >
            <option value={0}>Channel 0 (TIMER0)</option>
            <option value={1}>Channel 1 (TIMER1)</option>
          </select>
        </label>
      </div>

      <div
        style={{
          display: "grid",
          "grid-template-columns": "1fr 1fr",
          gap: "1rem",
          "margin-top": "1rem",
        }}
      >
        {/* Frequency Configuration */}
        <label style={{ display: "flex", "align-items": "center", gap: "0.5rem" }}>
          Frequency (Hz):
          <input
            type="number"
            value={frequency()}
            min={1}
            max={40000}
            onInput={(e) => setFrequency(Number(e.currentTarget.value))}
            style={{ width: "6em" }}
            title="PWM frequency in Hz (1-40000)"
          />
        </label>

        {/* Resolution Configuration */}
        <label style={{ display: "flex", "align-items": "center", gap: "0.5rem" }}>
          Resolution (bits):
          <select
            value={resolutionBits()}
            onChange={(e) => setResolutionBits(Number(e.currentTarget.value))}
            title="PWM resolution in bits (higher = more precision, lower frequency limit)"
          >
            <option value={8}>8 bits (0-255)</option>
            <option value={10}>10 bits (0-1023)</option>
            <option value={12}>12 bits (0-4095)</option>
            <option value={14}>14 bits (0-16383)</option>
            <option value={16}>16 bits (0-65535)</option>
          </select>
        </label>
      </div>
    </DeviceConfig>
  );
}
