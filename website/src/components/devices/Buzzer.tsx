import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { createSignal } from "solid-js";

interface IBuzzerState {
  playing: boolean;
  currentTune: string;
  pin: number;
  name: string;
  type: string;
}

export function Buzzer(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IBuzzerState>(props.id);

  // Local state for UI controls
  const [frequency, setFrequency] = createSignal(440); // Default to A4 note
  const [rtttl, setRtttl] = createSignal(
    "SuperMar:d=4,o=5,b=125:a,8f.,16c,16d,16f,16p,f,16d,16c,16p,16f,16p,16f,16p,8c6,8a.,g,16c,a,8f.,16c,16d,16f,16p,f,16d,16c,16p,16f,16p,16a#,16a,16g,2f,16p,8a.,8f.,8c,8a.,f,16g#,16f,16c,16p,8g#.,2g,8a.,8f.,8c,8a.,f,16g#,16f,8c,2c6"
  );

  const playTone = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "tone",
        frequency: frequency(),
        duration: 1000, // Default 1 second duration
      })
    );
  };

  const playMelody = () => {
    if (rtttl().trim().length === 0) {
      return;
    }

    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "tune",
        rtttl: rtttl(),
      })
    );
  };

  return (
    <fieldset>
      <legend>
        {deviceState()?.name || props.id} ({deviceState()?.type || "BUZZER"})
        {deviceState()?.pin && ` - Pin ${deviceState()?.pin}`}
      </legend>
      {disabled() && <span>{error() || connectedState() + ""}</span>}
      {!disabled() && (
        <>
          <div
            style={{
              "margin-bottom": "15px",
              display: "flex",
              "align-items": "center",
              gap: "8px",
            }}
          >
            <svg width="20" height="20" viewBox="0 0 20 20">
              <circle cx="10" cy="10" r="8" fill="#d3d3d3" stroke="#999" stroke-width="1" />
              <circle cx="10" cy="10" r="6" fill={deviceState()?.playing ? "#FF6B35" : "#e0e0e0"} />
              {deviceState()?.playing && <polygon points="8,7 8,13 14,10" fill="white" />}
            </svg>
            <span style={{ "font-weight": deviceState()?.playing ? "bold" : "normal" }}>
              {deviceState()?.playing ? "Playing" : "Idle"}
              {deviceState()?.currentTune && deviceState()?.playing && (
                <span style={{ "font-size": "12px", color: "#666", "margin-left": "8px" }}>
                  (Melody)
                </span>
              )}
            </span>
          </div>

          {/* Tone Section */}
          <div
            style={{
              "margin-bottom": "20px",
              padding: "10px",
              border: "1px solid #ddd",
              "border-radius": "5px",
            }}
          >
            <h4 style={{ margin: "0 0 10px 0", "font-size": "14px" }}>Tone Generator</h4>
            <div style={{ "margin-bottom": "10px" }}>
              <label
                for={`freq-${props.id}`}
                style={{ display: "block", "margin-bottom": "5px", "font-size": "12px" }}
              >
                Frequency: {frequency()}Hz
              </label>
              <input
                id={`freq-${props.id}`}
                type="range"
                min="100"
                max="2000"
                step="10"
                value={frequency()}
                onInput={(e) => setFrequency(Number(e.currentTarget.value))}
                style={{ width: "100%" }}
              />
              <div
                style={{
                  display: "flex",
                  "justify-content": "space-between",
                  "font-size": "10px",
                  color: "#666",
                }}
              >
                <span>100Hz</span>
                <span>2000Hz</span>
              </div>
            </div>
            <button
              onClick={playTone}
              disabled={disabled() || deviceState()?.playing}
              style={{
                padding: "8px 16px",
                "background-color": deviceState()?.playing ? "#ccc" : "#4CAF50",
                color: "white",
                border: "none",
                "border-radius": "4px",
                cursor: deviceState()?.playing ? "not-allowed" : "pointer",
              }}
            >
              Play Tone (1s)
            </button>
          </div>

          {/* Melody Section */}
          <div
            style={{
              "margin-bottom": "15px",
              padding: "10px",
              border: "1px solid #ddd",
              "border-radius": "5px",
            }}
          >
            <h4 style={{ margin: "0 0 10px 0", "font-size": "14px" }}>Melody Player (RTTTL)</h4>
            <div style={{ "margin-bottom": "10px" }}>
              <label
                for={`rtttl-${props.id}`}
                style={{ display: "block", "margin-bottom": "5px", "font-size": "12px" }}
              >
                RTTTL String:
              </label>
              <textarea
                id={`rtttl-${props.id}`}
                value={rtttl()}
                onInput={(e) => setRtttl(e.currentTarget.value)}
                placeholder="Enter RTTTL string (e.g., Nokia:d=4,o=5,b=140:8e6,8d6...)"
                style={{
                  width: "100%",
                  height: "60px",
                  padding: "5px",
                  "font-family": "monospace",
                  "font-size": "11px",
                  border: "1px solid #ccc",
                  "border-radius": "3px",
                  resize: "vertical",
                }}
              />
            </div>
            <div style={{ display: "flex", gap: "8px", "align-items": "center" }}>
              <button
                onClick={playMelody}
                disabled={disabled() || deviceState()?.playing || rtttl().trim().length === 0}
                style={{
                  padding: "8px 16px",
                  "background-color":
                    deviceState()?.playing || rtttl().trim().length === 0 ? "#ccc" : "#2196F3",
                  color: "white",
                  border: "none",
                  "border-radius": "4px",
                  cursor:
                    deviceState()?.playing || rtttl().trim().length === 0
                      ? "not-allowed"
                      : "pointer",
                }}
              >
                Play Melody
              </button>
              <button
                onClick={() =>
                  setRtttl(
                    "tetris:d=4,o=5,b=160:e6,8b,8c6,8d6,16e6,16d6,8c6,8b,a,8a,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,2a,8p,d6,8f6,a6,8g6,8f6,e6,8e6,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,a"
                  )
                }
                style={{
                  padding: "6px 12px",
                  "background-color": "#FF9800",
                  color: "white",
                  border: "none",
                  "border-radius": "4px",
                  cursor: "pointer",
                  "font-size": "12px",
                }}
              >
                Load Tetris
              </button>
            </div>
          </div>

          <div style={{ "font-size": "12px", color: "#666" }}>
            <div>Status: {deviceState()?.playing ? "Playing" : "Ready"}</div>
            {deviceState()?.currentTune && (
              <div style={{ "margin-top": "4px", "word-break": "break-all" }}>
                Current: {deviceState()?.currentTune.substring(0, 50)}...
              </div>
            )}
          </div>
        </>
      )}
    </fieldset>
  );
}
