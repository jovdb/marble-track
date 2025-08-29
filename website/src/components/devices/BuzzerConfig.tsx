
import { createSignal } from "solid-js";
import DeviceConfig from "./DeviceConfig";

interface BuzzerConfigProps {
  id: string;
}

export default function BuzzerConfig(props: BuzzerConfigProps) {
  const [pin, setPin] = createSignal(1);

  return (
    <DeviceConfig id={props.id} onSave={() => alert(`TODO: save`)}>
      <label>
        PWM Pin number:
        <input
          type="number"
          value={pin()}
          min={0}
          onInput={(e) => setPin(Number(e.currentTarget.value))}
          style={{ "margin-left": "0.5rem" }}
        />
      </label>
    </DeviceConfig>
  );
}
