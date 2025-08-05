import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";

interface IButtonState {
  pressed: boolean;
  pin: number;
  name: string;
  type: string;
}

export function Button(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IButtonState>(props.id);

  const handlePress = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "pressed",
      })
    );
  };

  const handleRelease = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "released",
      })
    );
  };

  return (
    <fieldset>
      <legend>
        <legend>Button: {deviceState()?.name || props.id}</legend>
      </legend>
      {disabled() && <span>{error() || connectedState() + ""}</span>}
      {!disabled() && (
        <>
          <div>
            <span style={{ "font-weight": deviceState()?.pressed ? "bold" : "normal" }}>
              {deviceState()?.pressed ? "Pressed" : "Released"}
            </span>
          </div>

          <div style={{ "margin-bottom": "10px" }}>
            <button
              onMouseDown={handlePress}
              onMouseUp={handleRelease}
              onMouseLeave={handleRelease}
              onTouchStart={handlePress}
              onTouchEnd={handleRelease}
              disabled={disabled()}
            >
              Virtual Button
            </button>
          </div>
        </>
      )}
    </fieldset>
  );
}
