import { onMount } from "solid-js";
import { useDevice } from "../../stores/Devices";

interface DeviceConfigProps {
  id: string;
  onSave: () => void;
  children?: any;
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
        {props.children}
        <div style={{ display: "flex", gap: "1rem", "justify-content": "flex-end" }}>
          <button type="submit">Save</button>
        </div>
      </form>
    </div>
  );
}

export function DeviceConfigTable(props: { children?: any }) {
  return <table>{props.children}</table>;
}

export function DeviceConfigRow(props: { children: any }) {
  return <tr>{props.children}</tr>;
}

export function DeviceConfigItem(props: { name: string; children: any }) {
  return (
    <>
      <td>{props.name}</td>
      <td>{props.children}</td>
    </>
  );
}
