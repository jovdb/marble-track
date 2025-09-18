import { JSX, onMount } from "solid-js";
import { useDevice } from "../../stores/Devices";

interface DeviceConfigProps {
  id: string;
  onSave: () => void;
  children?: JSX.Element | JSX.Element[];
}

export default function DeviceConfig(props: DeviceConfigProps) {
  const [, { getDeviceConfig }] = useDevice(props.id);

  onMount(() => {
    getDeviceConfig();
  });

  return (
    <div style={{ display: "flex", "flex-direction": "column", gap: "1rem" }}>
      <form
        onSubmit={(e) => {
          e.preventDefault();
          props.onSave();
        }}
      >
        <div>{props.children}</div>
        <div style={{ display: "flex", gap: "1rem", "justify-content": "flex-end" }}>
          <button type="submit">Save</button>
        </div>
      </form>
    </div>
  );
}
