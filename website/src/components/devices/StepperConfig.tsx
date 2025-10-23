import { For, Show, createEffect, createMemo, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { IStepperConfig, StepperType, STEPPER_TYPES, useStepper } from "../../stores/Stepper";

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
  const [defaultSpeed, setDefaultSpeed] = createSignal(500);
  const [defaultAcceleration, setDefaultAcceleration] = createSignal(150);

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

    if (cfg.stepperType === "DRIVER") {
      if (typeof cfg.stepPin === "number") {
        setStepPin(cfg.stepPin);
      }
      if (typeof cfg.dirPin === "number") {
        setDirPin(cfg.dirPin);
      }
      if (typeof cfg.enablePin === "number") {
        setEnablePin(cfg.enablePin);
      }
    } else {
      if (typeof cfg.pin1 === "number") {
        setPin1(cfg.pin1);
      }
      if (typeof cfg.pin2 === "number") {
        setPin2(cfg.pin2);
      }
      if (typeof cfg.pin3 === "number") {
        setPin3(cfg.pin3);
      }
      if (typeof cfg.pin4 === "number") {
        setPin4(cfg.pin4);
      }
      if (typeof cfg.enablePin === "number") {
        setEnablePin(cfg.enablePin);
      }
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

    if (typeof cfg.defaultSpeed === "number") {
      setDefaultSpeed(cfg.defaultSpeed);
    }
    if (typeof cfg.defaultAcceleration === "number") {
      setDefaultAcceleration(cfg.defaultAcceleration);
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
      defaultSpeed: Number(defaultSpeed()),
      defaultAcceleration: Number(defaultAcceleration()),
      invertEnable: invertEnable(),
    };

    if (isFourPin()) {
      payload.pin1 = Number(pin1());
      payload.pin2 = Number(pin2());
      payload.pin3 = Number(pin3());
      payload.pin4 = Number(pin4());
      if (enablePin() >= 0) {
        payload.enablePin = Number(enablePin());
      }
    } else {
      payload.stepPin = Number(stepPin());
      payload.dirPin = Number(dirPin());
      if (enablePin() >= 0) {
        payload.enablePin = Number(enablePin());
      }
    }

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
          <DeviceConfigRow>
            <DeviceConfigItem name="Enable pin">
              <input
                type="number"
                min={-1}
                value={enablePin()}
                onInput={(event) => setEnablePin(Number(event.currentTarget.value))}
              />
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
        <DeviceConfigRow>
          <DeviceConfigItem name="Default speed">
            <div>
              <input
                type="number"
                min={0}
                value={defaultSpeed()}
                onInput={(event) => setDefaultSpeed(Number(event.currentTarget.value))}
              />
              <span style={{ "margin-left": "0.5rem" }}>steps/s</span>
            </div>
          </DeviceConfigItem>
        </DeviceConfigRow>
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
          <DeviceConfigItem name="Default acceleration">
            <div>
              <input
                type="number"
                min={0}
                value={defaultAcceleration()}
                onInput={(event) => setDefaultAcceleration(Number(event.currentTarget.value))}
              />
              <span style={{ "margin-left": "0.5rem" }}>steps/s²</span>
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
