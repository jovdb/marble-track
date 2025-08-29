import { createSignal } from "solid-js";

interface LedConfigProps {
  id: string;
}

export default function LedConfig(props: LedConfigProps) {
  const [pin, setPin] = createSignal(1);

  return (
    <div style={{ display: "flex", "flex-direction": "column", gap: "1rem" }}>
      <label>
        Pin Number:
        <input
          type="number"
          value={pin()}
          min={0}
          onInput={(e) => setPin(Number(e.currentTarget.value))}
          style={{ "margin-left": "0.5rem" }}
        />
      </label>
      <div style={{ display: "flex", gap: "1rem", "justify-content": "flex-end" }}>
        <button
          type="button"
          onClick={() => {
            alert(`TODO: save`);
          }}
        >
          Save
        </button>
      </div>
    </div>
  );
}
