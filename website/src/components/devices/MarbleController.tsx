import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";

export function MarbleController(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  return (
    <Device
      id={props.id}
      icon={getDeviceIcon("marblecontroller")}
      stateComponent={() => null}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "1rem", "text-align": "center" }}>
        <p>Marble Controller - Manages lift, buzzer, and wheel operations</p>
      </div>
    </Device>
  );
}
