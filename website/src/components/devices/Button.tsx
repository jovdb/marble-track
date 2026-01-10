import { createMemo, createSignal } from "solid-js";
import { Device } from "./Device";
import deviceStyles from "./Device.module.css";
import { useButton } from "../../stores/Button";
import ButtonConfig from "./ButtonConfig";
import { ButtonIcon } from "../icons/Icons";

export function Button(props: { id: string }) {
  const buttonStore = useButton(props.id);
  const device = () => buttonStore[0];
  const actions = buttonStore[1];

  const isPressed = createMemo(() => Boolean(device()?.state?.isPressed));

  const [isPressing, setIsPressing] = createSignal(false);

  const isNC = createMemo(() => device()?.config?.buttonType === "NormalClosed");

  const displayPressed = createMemo(() => {
    const pressed = isPressed();
    return isNC() ? !pressed : pressed;
  });

  const handlePress = () => {
    if (!isPressing()) {
      setIsPressing(true);
      actions.press();
    }
  };

  const handleRelease = () => {
    if (isPressing()) {
      setIsPressing(false);
      actions.release();
    }
  };

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <ButtonConfig id={props.id} onClose={onClose} />}
      icon={<ButtonIcon />}
      stateComponent={() => null}
    >
      <div class={deviceStyles.device__controls}>
        <button
          classList={{
            [deviceStyles.device__button]: true,
            [deviceStyles["device__button--full"]]: true,
            [deviceStyles["device__button--secondary"]]: displayPressed(),
          }}
          onPointerDown={handlePress}
          onPointerUp={handleRelease}
          onPointerLeave={handleRelease}
          onPointerCancel={handleRelease}
        >
          {displayPressed() ? "Pressed" : "Released"}
        </button>
      </div>
    </Device>
  );
}
