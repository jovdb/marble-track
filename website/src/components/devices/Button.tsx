import { createMemo } from "solid-js";
import { Device } from "./Device";
import deviceStyles from "./Device.module.css";
import buttonStyles from "./Button.module.css";
import { useButton } from "../../stores/Button";
import ButtonConfig from "./ButtonConfig";
import { ButtonIcon } from "../icons/Icons";

export function Button(props: { id: string }) {
  const buttonStore = useButton(props.id);
  const device = () => buttonStore[0];
  const actions = buttonStore[1];

  const isPressed = createMemo(() => Boolean(device()?.state?.pressed));
  const statusLabel = createMemo(() => `Status: ${isPressed() ? "Pressed" : "Released"}`);

  const handlePress = () => actions.press();
  const handleRelease = () => actions.release();

  return (
    <Device
      id={props.id}
      deviceState={device()?.state}
      configComponent={(onClose) => <ButtonConfig id={props.id} onClose={onClose} />}
      icon={<ButtonIcon />}
    >
      <div class={deviceStyles.device__status}>
        <div
          classList={{
            [buttonStyles["button__status-indicator"]]: true,
            [buttonStyles["button__status-indicator--pressed"]]: isPressed(),
            [buttonStyles["button__status-indicator--off"]]: !isPressed(),
          }}
        ></div>
        <span class={deviceStyles["device__status-text"]}>{statusLabel()}</span>
      </div>
      <div class={deviceStyles.device__controls}>
        <button
          class={deviceStyles.device__button}
          onPointerDown={handlePress}
          onPointerUp={handleRelease}
          onPointerLeave={handleRelease}
          onPointerCancel={handleRelease}
        >
          Hold to Press
        </button>
      </div>
    </Device>
  );
}
