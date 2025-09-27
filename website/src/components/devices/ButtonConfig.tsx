import { createSignal } from "solid-js";
import DeviceConfig from "./DeviceConfig";

interface ButtonConfigProps {
  id: string;
  onClose: () => void;
}

export default function ButtonConfig(props: ButtonConfigProps) {
  const [pin, setPin] = createSignal(1);
  const [pinMode, setPinMode] = createSignal("floating");
  const [debounce, setDebounce] = createSignal(50);

  return (
    <DeviceConfig id={props.id} onSave={() => alert(`TODO: save`)} onClose={props.onClose}>
      <label>
        Pin number:
        <input
          type="number"
          value={pin()}
          min={0}
          onInput={(e) => setPin(Number(e.currentTarget.value))}
          style={{ "margin-left": "0.5rem" }}
        />
      </label>
      <label style={{ display: "block", "margin-top": "1em" }}>
        Pin mode:
        <select
          value={pinMode()}
          onInput={(e) => setPinMode(e.currentTarget.value)}
          style={{ "margin-left": "0.5rem" }}
        >
          <option value="floating">Floating</option>
          <option value="pullup">Pull-up</option>
          <option value="pulldown">Pull-down</option>
        </select>
      </label>
      <label style={{ display: "block", "margin-top": "1em" }}>
        Debounce time (ms):
        <input
          type="number"
          value={debounce()}
          min={0}
          onInput={(e) => setDebounce(Number(e.currentTarget.value))}
          style={{ "margin-left": "0.5rem", width: "4em" }}
        />
      </label>
    </DeviceConfig>
  );
}
