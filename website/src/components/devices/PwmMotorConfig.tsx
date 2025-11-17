import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
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
  const [mcpwmChannel, setMcpwmChannel] = createSignal<number>(device()?.config?.mcpwmChannel ?? -1);
  const [frequency, setFrequency] = createSignal<number>(device()?.config?.frequency ?? 50);
  const [resolutionBits, setResolutionBits] = createSignal<number>(
    device()?.config?.resolutionBits ?? 12
  );
  const [minDutyCycle, setMinDutyCycle] = createSignal<number>(device()?.config?.minDutyCycle ?? 5);
  const [maxDutyCycle, setMaxDutyCycle] = createSignal<number>(
    device()?.config?.maxDutyCycle ?? 10
  );
  const [defaultDurationInMs, setDefaultDurationInMs] = createSignal<number>(
    device()?.config?.defaultDurationInMs ?? 500
  );

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

    if (typeof config.mcpwmChannel === "number") {
      setMcpwmChannel(config.mcpwmChannel);
    }

    if (typeof config.frequency === "number") {
      setFrequency(config.frequency);
    }

    if (typeof config.resolutionBits === "number") {
      setResolutionBits(config.resolutionBits);
    }

    if (typeof config.minDutyCycle === "number") {
      setMinDutyCycle(config.minDutyCycle);
    }

    if (typeof config.maxDutyCycle === "number") {
      setMaxDutyCycle(config.maxDutyCycle);
    }

    if (typeof config.defaultDurationInMs === "number") {
      setDefaultDurationInMs(config.defaultDurationInMs);
    }
  });

  return (
    <DeviceConfig
      device={device()}
      onSave={() =>
        actions.setDeviceConfig({
          name: name()?.trim() || device()?.id,
          pin: pin(),
          mcpwmChannel: mcpwmChannel(),
          frequency: frequency(),
          resolutionBits: resolutionBits(),
          minDutyCycle: minDutyCycle(),
          maxDutyCycle: maxDutyCycle(),
          defaultDurationInMs: defaultDurationInMs(),
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
          <DeviceConfigItem name="Pin:">
            <input
              type="number"
              value={pin()}
              min={-1}
              max={50}
              onInput={(event) => setPin(Number(event.currentTarget.value))}
              style={{ width: "4em", "margin-left": "0.5rem" }}
              title="GPIO pin number for PWM output (-1 to disable, 0-39)"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="MCPWM Channel:">
            <select
              value={mcpwmChannel()}
              onChange={(event) => setMcpwmChannel(Number(event.currentTarget.value))}
              title="MCPWM channel (-1 = auto-acquire, 0-5 = specific channel)"
              style={{ "margin-left": "0.5rem" }}
            >
              <option value={-1}>Auto-acquire</option>
              <option value={0}>MCPWM_OUT0A</option>
              <option value={1}>MCPWM_OUT0B</option>
              <option value={2}>MCPWM_OUT1A</option>
              <option value={3}>MCPWM_OUT1B</option>
              <option value={4}>MCPWM_OUT2A</option>
              <option value={5}>MCPWM_OUT2B</option>
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
        <DeviceConfigRow>
          <DeviceConfigItem name="Duty Cycle at 0%">
            <input
              type="number"
              value={minDutyCycle().toFixed(1)}
              min={0}
              max={100}
              step={0.1}
              onInput={(event) => setMinDutyCycle(Number(event.currentTarget.value))}
              style={{ width: "5em", "margin-left": "0.5rem" }}
              title="Duty cycle percentage when value is 0% (0.0-100.0)"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Duty Cycle at 100%">
            <input
              type="number"
              value={maxDutyCycle().toFixed(1)}
              min={0}
              max={100}
              step={0.1}
              onInput={(event) => setMaxDutyCycle(Number(event.currentTarget.value))}
              style={{ width: "5em", "margin-left": "0.5rem" }}
              title="Duty cycle percentage when value is 100% (0.0-100.0)"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Default Duration (ms)">
            <input
              type="number"
              value={defaultDurationInMs()}
              min={0}
              step={10}
              onInput={(event) => setDefaultDurationInMs(Number(event.currentTarget.value))}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="Default duration in milliseconds (0 = indefinite)"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
