import { createSignal } from "solid-js";
import DeviceConfig from "./DeviceConfig";

interface LedConfigProps {
  id: string;
}

export default function LedConfig(props: LedConfigProps) {
  const [name, setName] = createSignal("Led");
  const [pin, setPin] = createSignal(1);

  return (
    <DeviceConfig
      id={props.id}
      onSave={() => {
        alert("TODO: Implement LED setup");
      }}
    >
      <>
        <div>
          <label>
            Name:
            <input
              type="text"
              value={name()}
              onInput={(e) => setName(e.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            />
          </label>
        </div>
        <div>
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
        </div>
      </>
    </DeviceConfig>
  );
}
