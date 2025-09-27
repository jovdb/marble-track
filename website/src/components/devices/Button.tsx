import { Device } from "./Device";
import deviceStyles from "./Device.module.css";
import buttonStyles from "./Button.module.css";
import { createButtonStore } from "../../stores/Button";
import ButtonConfig from "./ButtonConfig";
import { ButtonIcon } from "../icons/Icons";

export function Button(props: { id: string }) {
  const { state, error, press, release } = createButtonStore(props.id);

  const handlePress = () => {
    press();
  };

  const handleRelease = () => {
    release();
  };

  return (
    <Device
      id={props.id}
      deviceState={state()}
      configComponent={(onClose) => <ButtonConfig id={props.id} onClose={onClose} />}
      icon={<ButtonIcon />}
    >
      {error() && <div class={deviceStyles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={deviceStyles.device__status}>
            <div
              classList={{
                [buttonStyles["button__status-indicator"]]: true,
                [buttonStyles["button__status-indicator--pressed"]]: state()?.pressed,
                [buttonStyles["button__status-indicator--off"]]: !state()?.pressed,
              }}
            ></div>
            <span class={deviceStyles["device__status-text"]}>
              Status: {state()?.pressed ? "Pressed" : "Released"}
            </span>
          </div>
          <div class={deviceStyles.device__controls}>
            <button
              class={deviceStyles.device__button}
              onMouseDown={handlePress}
              onMouseUp={handleRelease}
            >
              Hold to Press
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
