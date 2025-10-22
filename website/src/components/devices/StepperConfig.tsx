import { For, Show, createEffect, createMemo, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import {
  IStepperConfig,
  IStepperDriverPins,
  IStepperFourWirePins,
  StepperType,
  STEPPER_TYPES,
  useStepper,
} from "../../stores/Stepper";

const STEPPER_TYPE_OPTIONS: { value: StepperType; label: string }[] = [
  { value: "DRIVER", label: "Driver (2-pin)" },
  { value: "HALF4WIRE", label: "Half 4-wire" },
  { value: "FULL4WIRE", label: "Full 4-wire" },
];

export default function StepperConfig(props: { id: string; onClose: () => void }) {
  const stepperStore = useStepper(props.id);
  const device = () => stepperStore[0];
  const { setDeviceConfig } = stepperStore[1];
  const config = createMemo(() => device()?.config as IStepperConfig | undefined);

  const [name, setName] = createSignal("Stepper");
  const [stepperType, setStepperType] = createSignal<StepperType>("DRIVER");
  const [stepPin, setStepPin] = createSignal(1);
  const [dirPin, setDirPin] = createSignal(2);
  const [enablePin, setEnablePin] = createSignal(-1);
  const [invertEnable, setInvertEnable] = createSignal(false);
  const [pin1, setPin1] = createSignal(1);
  const [pin2, setPin2] = createSignal(2);
  const [pin3, setPin3] = createSignal(3);
  const [pin4, setPin4] = createSignal(4);
  const [maxSpeed, setMaxSpeed] = createSignal(1000);
  const [maxAcceleration, setMaxAcceleration] = createSignal(300);

  createEffect(() => {
    const cfg = config();
    if (!cfg) {
      return;
    }

    if (typeof cfg.name === "string" && cfg.name.length > 0) {
      setName(cfg.name);
    }

    if (cfg.stepperType && STEPPER_TYPES.some((type) => type === cfg.stepperType)) {
      setStepperType(cfg.stepperType as StepperType);
    } else {
      setStepperType("DRIVER");
    }

    if (Array.isArray(cfg.pins)) {
      const pins = cfg.pins as IStepperFourWirePins;
      setPin1(typeof pins[0] === "number" ? pins[0] : pin1());
      setPin2(typeof pins[1] === "number" ? pins[1] : pin2());
      setPin3(typeof pins[2] === "number" ? pins[2] : pin3());
      setPin4(typeof pins[3] === "number" ? pins[3] : pin4());
    } else if (cfg.pins && typeof cfg.pins === "object") {
      const pins = cfg.pins as IStepperDriverPins;
      setStepPin(typeof pins.stepPin === "number" ? pins.stepPin : stepPin());
      setDirPin(typeof pins.dirPin === "number" ? pins.dirPin : dirPin());
    }

    if (typeof cfg.enablePin === "number") {
      setEnablePin(cfg.enablePin);
    }

    if (typeof cfg.invertEnable === "boolean") {
      setInvertEnable(cfg.invertEnable);
    }

    if (typeof cfg.maxSpeed === "number") {
      setMaxSpeed(cfg.maxSpeed);
    }
    const cfgMaxAcceleration =
      typeof cfg.maxAcceleration === "number"
        ? cfg.maxAcceleration
        : (cfg as { acceleration?: number } | undefined)?.acceleration;
    if (typeof cfgMaxAcceleration === "number") {
      setMaxAcceleration(cfgMaxAcceleration);
    }
  });

  const isFourPin = createMemo(() => {
    const type = stepperType();
    return type === "HALF4WIRE" || type === "FULL4WIRE";
  });

  const handleSave = () => {
    const payload: IStepperConfig = {
      name: name().trim() || device()?.id,
      stepperType: stepperType(),
      maxSpeed: Number(maxSpeed()),
      maxAcceleration: Number(maxAcceleration()),
      pins: isFourPin()
        ? ([pin1(), pin2(), pin3(), pin4()] as IStepperFourWirePins)
        : ({ stepPin: stepPin(), dirPin: dirPin() } as IStepperDriverPins),
      enablePin: enablePin() >= 0 ? enablePin() : undefined,
      invertEnable: invertEnable(),
    };

    setDeviceConfig(payload);
  };

  return (
    <DeviceConfig device={device()} onSave={handleSave} onClose={props.onClose}>
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name">
            <input
              type="text"
              value={name()}
              onInput={(event) => setName(event.currentTarget.value)}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Stepper type">
            <select
              value={stepperType()}
              onChange={(event) => setStepperType(event.currentTarget.value as StepperType)}
            >
              <For each={STEPPER_TYPE_OPTIONS}>
                {(option) => <option value={option.value}>{option.label}</option>}
              </For>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <Show when={!isFourPin()}>
          <DeviceConfigRow>
            <DeviceConfigItem name="Step pin">
              <input
                type="number"
                min={0}
                value={stepPin()}
                onInput={(event) => setStepPin(Number(event.currentTarget.value))}
              />
            </DeviceConfigItem>
          </DeviceConfigRow>
          <DeviceConfigRow>
            <DeviceConfigItem name="Direction pin">
              <input
                type="number"
                min={0}
                value={dirPin()}
                onInput={(event) => setDirPin(Number(event.currentTarget.value))}
              />
            </DeviceConfigItem>
          </DeviceConfigRow>
          <DeviceConfigRow>
            <DeviceConfigItem name="Enable pin">
              <input
                type="number"
                min={-1}
                value={enablePin()}
                onInput={(event) => setEnablePin(Number(event.currentTarget.value))}
              />
              <span
                style={{ "margin-left": "0.5rem", "font-size": "0.8rem", color: "#666" }}
              ></span>
            </DeviceConfigItem>
          </DeviceConfigRow>
          <DeviceConfigRow>
            <DeviceConfigItem name="Invert enable">
              <input
                type="checkbox"
                checked={invertEnable()}
                onChange={(event) => setInvertEnable(event.currentTarget.checked)}
              />
            </DeviceConfigItem>
          </DeviceConfigRow>
        </Show>
        <Show when={isFourPin()}>
          <DeviceConfigRow>
            <DeviceConfigItem name="Pin 1">
              <input
                type="number"
                min={0}
                value={pin1()}
                onInput={(event) => setPin1(Number(event.currentTarget.value))}
              />
            </DeviceConfigItem>
          </DeviceConfigRow>
          <DeviceConfigRow>
            <DeviceConfigItem name="Pin 2">
              <input
                type="number"
                min={0}
                value={pin2()}
                onInput={(event) => setPin2(Number(event.currentTarget.value))}
              />
            </DeviceConfigItem>
          </DeviceConfigRow>
          <DeviceConfigRow>
            <DeviceConfigItem name="Pin 3">
              <input
                type="number"
                min={0}
                value={pin3()}
                onInput={(event) => setPin3(Number(event.currentTarget.value))}
              />
            </DeviceConfigItem>
          </DeviceConfigRow>
          <DeviceConfigRow>
            <DeviceConfigItem name="Pin 4">
              <input
                type="number"
                min={0}
                value={pin4()}
                onInput={(event) => setPin4(Number(event.currentTarget.value))}
              />
            </DeviceConfigItem>
          </DeviceConfigRow>
        </Show>
        <DeviceConfigRow>
          <DeviceConfigItem name="Max speed">
            <div>
              <input
                type="number"
                min={0}
                value={maxSpeed()}
                onInput={(event) => setMaxSpeed(Number(event.currentTarget.value))}
              />
              <span style={{ "margin-left": "0.5rem" }}>steps/s</span>
            </div>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Max acceleration">
            <div>
              <input
                type="number"
                min={0}
                value={maxAcceleration()}
                onInput={(event) => setMaxAcceleration(Number(event.currentTarget.value))}
              />
              <span style={{ "margin-left": "0.5rem" }}>steps/s²</span>
            </div>
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
