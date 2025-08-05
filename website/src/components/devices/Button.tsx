import { createSignal } from "solid-js";
import { sendMessage } from "../../hooks/useWebSocket";

export function Button(props: { id: string }) {
  const [isPressed, setIsPressed] = createSignal(false);

  const handlePress = () => {
    setIsPressed(true);
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "pressed",
      })
    );
  };

  const handleRelease = () => {
    setIsPressed(false);
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
      <legend>Button: {props.id}</legend>
      <div
        style={{
          "margin-bottom": "10px",
          display: "flex",
          "align-items": "center",
          gap: "8px",
        }}
      >
        <span
          style={{
            "font-weight": isPressed() ? "bold" : "normal",
          }}
        >
          {isPressed() ? "Pressed" : "Released"}
        </span>
      </div>

      <div style={{ "margin-bottom": "10px" }}>
        <button
          onMouseDown={handlePress}
          onMouseUp={handleRelease}
          onMouseLeave={handleRelease}
          onTouchStart={handlePress}
          onTouchEnd={handleRelease}
        >
          Virtual Button
        </button>
      </div>
    </fieldset>
  );
}
