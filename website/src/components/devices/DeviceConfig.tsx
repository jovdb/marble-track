import { JSX } from "solid-js";

interface DeviceConfigProps {
  id: string;
  onSave: () => void;
  children?: JSX.Element | JSX.Element[];
}

export default function DeviceConfig(props: DeviceConfigProps) {
  return (
    <div style={{ display: "flex", "flex-direction": "column", gap: "1rem" }}>
      <div>{props.children}</div>
      <div style={{ display: "flex", gap: "1rem", "justify-content": "flex-end" }}>
        <button type="button" onClick={props.onSave}>
          Save
        </button>
      </div>
    </div>
  );
}
