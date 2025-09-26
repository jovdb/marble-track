import { For, createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { LED_INITIAL_STATES, LedInitialState, useLed } from "../../stores/Led";

interface LedConfigProps {
  id: string;
}

export default function LedConfig(props: LedConfigProps) {
  const [device, { setDeviceConfig }] = useLed(props.id);
  const normalizeInitialState = (value: unknown): LedInitialState => {
    const candidate = value as LedInitialState;
    return LED_INITIAL_STATES.includes(candidate) ? candidate : "OFF";
  };

  const [name, setName] = createSignal(device?.config?.name ?? "Led");
  const [pin, setPin] = createSignal(device?.config?.pin ?? -1);
  const [initialState, setInitialState] = createSignal<LedInitialState>(
    normalizeInitialState(device?.config?.initialState)
  );

  createEffect(() => {
    const config = device?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }
    if (typeof config.pin === "number") {
      setPin(config.pin);
    }
    const nextInitialState = normalizeInitialState(config.initialState);
    setInitialState(nextInitialState);
  });

  return (
    <DeviceConfig
      id={props.id}
      onSave={() => {
        setDeviceConfig({
          name: name(),
          pin: pin(),
          initialState: initialState(),
        });
      }}
    >
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name:">
            <input
              type="text"
              value={name() || ""}
              onInput={(e) => setName(e.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Pin:">
            <input
              type="number"
              value={pin() || "1"}
              min={1}
              max={50}
              onInput={(e) => setPin(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Initial state:">
            <select
              value={initialState()}
              onChange={(event) =>
                setInitialState(normalizeInitialState(event.currentTarget.value))
              }
              style={{ "margin-left": "0.5rem" }}
            >
              <For each={LED_INITIAL_STATES}>
                {(option) => <option value={option}>{option}</option>}
              </For>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
