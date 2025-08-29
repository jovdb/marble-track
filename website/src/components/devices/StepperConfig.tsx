import { createSignal } from "solid-js";
import DeviceConfig from "./DeviceConfig";

interface StepperConfigProps {
  id: string;
}

export default function StepperConfig(props: StepperConfigProps) {
  const [stepPin, setStepPin] = createSignal(1);
  const [dirPin, setDirPin] = createSignal(2);
  const [maxSpeed, setMaxSpeed] = createSignal(1000);
  const [acceleration, setAcceleration] = createSignal(100);

  return (
    <DeviceConfig id={props.id} onSave={() => alert(`TODO: save`)}>
      <label>
        Step pin:
        <input
          type="number"
          value={stepPin()}
          min={0}
          onInput={(e) => setStepPin(Number(e.currentTarget.value))}
          style={{ "margin-left": "0.5rem", width: "4em" }}
        />
      </label>
      <label style={{ display: "block", "margin-top": "1em" }}>
        Direction pin:
        <input
          type="number"
          value={dirPin()}
          min={0}
          onInput={(e) => setDirPin(Number(e.currentTarget.value))}
          style={{ "margin-left": "0.5rem", width: "4em" }}
        />
      </label>
      <label style={{ display: "block", "margin-top": "1em" }}>
        Max speed:
        <input
          type="number"
          value={maxSpeed()}
          min={0}
          onInput={(e) => setMaxSpeed(Number(e.currentTarget.value))}
          style={{ "margin-left": "0.5rem", width: "6em" }}
        />
      </label>
      <label style={{ display: "block", "margin-top": "1em" }}>
        Max acceleration:
        <input
          type="number"
          value={acceleration()}
          min={0}
          onInput={(e) => setAcceleration(Number(e.currentTarget.value))}
          style={{ "margin-left": "0.5rem", width: "6em" }}
        />
      </label>
    </DeviceConfig>
  );
}
