import { createEffect, createSignal } from "solid-js";
import DeviceConfig, {
  DeviceConfigItem,
  DeviceConfigRow,
  DeviceConfigTable,
} from "./DeviceConfig";
import { usePwmMotor } from "../../stores/PwmMotor";

interface PwmMotorConfigProps {
  id: string;
  onClose: () => void;
}

export default function PwmMotorConfig(props: PwmMotorConfigProps) {
  const pwmMotorStore = usePwmMotor(props.id);
  const device = () => pwmMotorStore[0];
  const actions = pwmMotorStore[1];

  const [name, setName] = createSignal(device()?.config?.name ?? device()?.id ?? "PWM Motor");
  const [pin, setPin] = createSignal<number>(device()?.config?.pin ?? -1);
  const [pwmChannel, setPwmChannel] = createSignal<number>(device()?.config?.pwmChannel ?? 0);
  const [frequency, setFrequency] = createSignal<number>(device()?.config?.frequency ?? 5000);
  const [resolutionBits, setResolutionBits] = createSignal<number>(device()?.config?.resolutionBits ?? 12);

  createEffect(() => {
    const config = device()?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }

    if (typeof config.pin === "number") {
      setPin(config.pin);
    }

    if (typeof config.pwmChannel === "number") {
      setPwmChannel(config.pwmChannel);
    }

    if (typeof config.frequency === "number") {
      setFrequency(config.frequency);
    }

    if (typeof config.resolutionBits === "number") {
      setResolutionBits(config.resolutionBits);
    }
  });

  return (
    <DeviceConfig
      id={props.id}
      onSave={() =>
        actions.setDeviceConfig({
          name: name()?.trim() || device()?.id,
          pin: pin(),
          pwmChannel: pwmChannel(),
          frequency: frequency(),
          resolutionBits: resolutionBits(),
        })
      }
      onClose={props.onClose}
    >
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name:">
            <input
              type="text"
              value={name() || ""}
              onInput={(event) => setName(event.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="GPIO Pin:">
            <input
              type="number"
              value={pin()}
              min={0}
              max={39}
              onInput={(event) => setPin(Number(event.currentTarget.value))}
              style={{ width: "4em", "margin-left": "0.5rem" }}
              title="GPIO pin number for PWM output (0-39)"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="PWM Channel:">
            <select
              value={pwmChannel()}
              onChange={(event) => setPwmChannel(Number(event.currentTarget.value))}
              title="MCPWM channel (0 = MCPWM0A/TIMER0, 1 = MCPWM1A/TIMER1)"
              style={{ "margin-left": "0.5rem" }}
            >
              <option value={0}>Channel 0 (TIMER0)</option>
              <option value={1}>Channel 1 (TIMER1)</option>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Frequency (Hz):">
            <input
              type="number"
              value={frequency()}
              min={1}
              max={40000}
              onInput={(event) => setFrequency(Number(event.currentTarget.value))}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="PWM frequency in Hz (1-40000)"
            />
            (10Hz to 40MHz)
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Resolution (bits):">
            <select
              value={resolutionBits()}
              onChange={(event) => setResolutionBits(Number(event.currentTarget.value))}
              title="PWM resolution in bits (higher = more precision, lower frequency limit)"
              style={{ "margin-left": "0.5rem" }}
            >
              <option value={8}>8 bits (0-255)</option>
              <option value={10}>10 bits (0-1023)</option>
              <option value={12}>12 bits (0-4095)</option>
              <option value={14}>14 bits (0-16383)</option>
              <option value={16}>16 bits (0-65535)</option>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
