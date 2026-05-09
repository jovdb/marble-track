import { createEffect, createMemo, createSignal, Show } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useServo } from "../../stores/Servo";
import { useDevices } from "../../stores/Devices";
import PinSelect from "../PinSelect";
import { PinConfig, deserializePinConfig } from "../../interfaces/WebSockets";

interface ServoConfigProps {
  id: string;
  onClose: () => void;
}

export default function ServoConfig(props: ServoConfigProps) {
  const servoStore = useServo(props.id);
  const device = () => servoStore[0];
  const actions = servoStore[1];
  const [devicesState] = useDevices();

  const [name, setName] = createSignal(device()?.config?.name ?? device()?.id ?? "Servo");
  const [pin, setPin] = createSignal<PinConfig>(deserializePinConfig(device()?.config?.pin ?? -1));

  // True when the selected pin lives on a PwmExpander — MCPWM/frequency/resolution don't apply
  const isPwmExpanderPin = createMemo(() => !!pin().expanderId);

  const [mcpwmChannel, setMcpwmChannel] = createSignal<number>(
    device()?.config?.mcpwmChannel ?? -1
  );
  const [frequency, setFrequency] = createSignal<string>(String(device()?.config?.frequency ?? 50));
  const [resolutionBits, setResolutionBits] = createSignal<number>(
    device()?.config?.resolutionBits ?? 10
  );
  const [minDutyCycle, setMinDutyCycle] = createSignal<string>(
    String(device()?.config?.minDutyCycle ?? 2.5)
  );
  const [maxDutyCycle, setMaxDutyCycle] = createSignal<string>(
    String(device()?.config?.maxDutyCycle ?? 12.5)
  );
  const [defaultDurationInMs, setDefaultDurationInMs] = createSignal<string>(
    String(device()?.config?.defaultDurationInMs ?? 500)
  );

  const toNumber = (value: string, fallback = 0) => {
    const num = Number(value);
    return Number.isFinite(num) ? num : fallback;
  };

  // Convert a duty cycle percentage to pulse width in ms given the current frequency
  const dutyToMs = (dutyCycle: string) => {
    // For PwmExpander pins, use the expander device's configured frequency
    let freq: number;
    if (isPwmExpanderPin()) {
      const expanderDevice = devicesState.devices[pin().expanderId];
      freq = (expanderDevice?.config?.frequency as number | undefined) ?? 50;
    } else {
      freq = toNumber(frequency(), 50);
    }
    if (freq <= 0) return null;
    const ms = (toNumber(dutyCycle, 0) / 100) * (1000 / freq);
    return ms.toFixed(2);
  };

  createEffect(() => {
    const config = device()?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }

    if (typeof config.pin === "number" || typeof config.pin === "object") {
      setPin(deserializePinConfig(config.pin));
    }
    if (typeof config.mcpwmChannel === "number") {
      setMcpwmChannel(config.mcpwmChannel);
    }

    if (typeof config.frequency === "number") {
      setFrequency(String(config.frequency));
    }

    if (typeof config.resolutionBits === "number") {
      setResolutionBits(config.resolutionBits);
    }

    if (typeof config.minDutyCycle === "number") {
      setMinDutyCycle(config.minDutyCycle.toFixed(1));
    }

    if (typeof config.maxDutyCycle === "number") {
      setMaxDutyCycle(config.maxDutyCycle.toFixed(1));
    }

    if (typeof config.defaultDurationInMs === "number") {
      setDefaultDurationInMs(String(config.defaultDurationInMs));
    }
  });

  return (
    <DeviceConfig
      device={device()}
      onSave={() =>
        actions.setDeviceConfig({
          name: name()?.trim() || device()?.id,
          pin: pin(),
          mcpwmChannel: isPwmExpanderPin() ? -1 : mcpwmChannel(),
          frequency: isPwmExpanderPin() ? toNumber(frequency(), 50) : toNumber(frequency(), 50),
          resolutionBits: resolutionBits(),
          minDutyCycle: toNumber(minDutyCycle(), 2.5),
          maxDutyCycle: toNumber(maxDutyCycle(), 12.5),
          defaultDurationInMs: toNumber(defaultDurationInMs(), 500),
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
            <PinSelect
              value={pin()}
              onChange={setPin}
              style={{ "margin-left": "0.5rem" }}
              title="GPIO pin or PwmExpander channel for servo PWM output"
              excludeDeviceId={props.id}
              showPwmExpanderPins={true}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <Show when={!isPwmExpanderPin()}>
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
        </Show>
        <DeviceConfigRow>
          <DeviceConfigItem name="Frequency (Hz):">
            <input
              type="number"
              value={
                isPwmExpanderPin()
                  ? ((devicesState.devices[pin().expanderId]?.config?.frequency as
                      | number
                      | undefined) ?? 50)
                  : frequency()
              }
              min={1}
              max={isPwmExpanderPin() ? 1526 : 400}
              onInput={(event) => setFrequency(event.currentTarget.value)}
              style={{ width: "5em", "margin-left": "0.5rem" }}
              title={
                isPwmExpanderPin()
                  ? "Frequency set by the PwmExpander device"
                  : "PWM frequency in Hz (typically 50Hz for servos)"
              }
              disabled={isPwmExpanderPin()}
            />
            <Show when={isPwmExpanderPin()}>
              <span style={{ "margin-left": "0.4em", opacity: 0.6, "font-size": "0.85em" }}>
                (set by PwmExpander)
              </span>
            </Show>
            <Show when={!isPwmExpanderPin()}>
              <span style={{ "margin-left": "0.4em" }}>(typically 50Hz for servos)</span>
            </Show>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Resolution (bits):">
            <select
              value={isPwmExpanderPin() ? 12 : resolutionBits()}
              onChange={(event) => setResolutionBits(Number(event.currentTarget.value))}
              title={
                isPwmExpanderPin()
                  ? "PCA9685 is always 12-bit resolution"
                  : "PWM resolution in bits (higher = more precision)"
              }
              style={{ "margin-left": "0.5rem" }}
              disabled={isPwmExpanderPin()}
            >
              <option value={8}>8 bits (0-255)</option>
              <option value={10}>10 bits (0-1023)</option>
              <option value={12}>12 bits (0-4095)</option>
              <option value={14}>14 bits (0-16383)</option>
              <option value={16}>16 bits (0-65535)</option>
            </select>
            <Show when={isPwmExpanderPin()}>
              <span style={{ "margin-left": "0.4em", opacity: 0.6, "font-size": "0.85em" }}>
                (fixed by PwmExpander)
              </span>
            </Show>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Min Duty Cycle (0°):">
            <input
              type="number"
              value={minDutyCycle()}
              min={0}
              max={100}
              step={0.1}
              onInput={(event) => setMinDutyCycle(event.currentTarget.value)}
              style={{ width: "5em", "margin-left": "0.5rem" }}
              title="Duty cycle percentage for 0° position (typically 2.5%)"
            />
            %
            <span style={{ "margin-left": "0.4em", opacity: 0.6, "font-size": "0.85em" }}>
              = {dutyToMs(minDutyCycle())} ms
            </span>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Max Duty Cycle (180°):">
            <input
              type="number"
              value={maxDutyCycle()}
              min={0}
              max={100}
              step={0.1}
              onInput={(event) => setMaxDutyCycle(event.currentTarget.value)}
              style={{ width: "5em", "margin-left": "0.5rem" }}
              title="Duty cycle percentage for 180° position (typically 12.5%)"
            />
            %
            <span style={{ "margin-left": "0.4em", opacity: 0.6, "font-size": "0.85em" }}>
              = {dutyToMs(maxDutyCycle())} ms
            </span>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Default Duration (ms):">
            <input
              type="number"
              value={defaultDurationInMs()}
              min={0}
              step={10}
              onInput={(event) => setDefaultDurationInMs(event.currentTarget.value)}
              style={{ width: "6em", "margin-left": "0.5rem" }}
              title="Default animation duration in milliseconds"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
