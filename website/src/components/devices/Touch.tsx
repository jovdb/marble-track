import { createMemo, createSignal } from "solid-js";
import { Device } from "./Device";
import deviceStyles from "./Device.module.css";
import TouchConfig from "./TouchConfig";
import { TouchStateIcon } from "./TouchStateIcon";
import { useTouch } from "../../stores/Touch";

export function Touch(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const touchStore = useTouch(props.id);
  const device = () => touchStore[0];
  const actions = touchStore[1];

  const touched = createMemo(() => Boolean(device()?.state?.touched));
  const [isPressing, setIsPressing] = createSignal(false);

  const handleTouch = () => {
    if (!isPressing()) {
      setIsPressing(true);
      actions.touch();
    }
  };

  const handleUntouch = () => {
    if (isPressing()) {
      setIsPressing(false);
      actions.untouch();
    }
  };

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <TouchConfig id={props.id} onClose={onClose} />}
      icon={<TouchStateIcon deviceId={props.id} />}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div class={deviceStyles.device__controls}>
        <button
          classList={{
            [deviceStyles.device__button]: true,
            [deviceStyles["device__button--full"]]: true,
            [deviceStyles["device__button--secondary"]]: touched(),
          }}
          onPointerDown={handleTouch}
          onPointerUp={handleUntouch}
          onPointerLeave={handleUntouch}
          onPointerCancel={handleUntouch}
          onContextMenu={(event) => event.preventDefault()}
        >
          {touched() ? "Touched" : "Untouched"}
        </button>
      </div>
    </Device>
  );
}
