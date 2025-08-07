import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { createSignal, For } from "solid-js";

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
    "Pacman:d=4,o=5,b=112:32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32c6,32p,32c7,32p,32g6,32p,32e6,32p,32c7,32g6,16p,16e6,16p,32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32d#6,32e6,32f6,32p,32f6,32f#6,32g6,32p,32g6,32g#6,32a6,32p,32b.6"
  );

  // Predefined RTTTL tunes
  const rtttlTunes = {
    "Tetris": "tetris:d=4,o=5,b=160:e6,8b,8c6,8d6,16e6,16d6,8c6,8b,a,8a,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,2a,8p,d6,8f6,a6,8g6,8f6,e6,8e6,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,a",
    "Nokia": "nokia:d=4,o=5,b=125:8e6,8d6,8f#,8g#,8c#6,8b,8d,8e,8b,8a,8c#,8e,2a",
    "Super Mario": "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p",
    "Star Wars": "starwars:d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6",
    "Pacman": "Pacman:d=4,o=5,b=112:32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32c6,32p,32c7,32p,32g6,32p,32e6,32p,32c7,32g6,16p,16e6,16p,32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32d#6,32e6,32f6,32p,32f6,32f#6,32g6,32p,32g6,32g#6,32a6,32p,32b.6",
    "Indiana Jones": "indiana:d=4,o=5,b=250:e,8p,8f,8g,8p,1c6,8p.,d,8p,8e,1f,p.,g,8p,8a,8b,8p,1f6,p,a,8p,8b,2c6,2d6,2e6,e,8p,8f,8g,8p,1c6,p,d6,8p,8e6,1f.6,g,8p,8g,e.6,8p,d6,8p,8g,e.6,8p,d6,8p,8g,f.6,8p,e6,8p,8d6,2c6",
    "Imperial March": "Imperial:d=4,o=5,b=120:8g,8g,8g,8d#,16p,16a#,8g,8d#,16p,16a#,2g,8p,8d6,8d6,8d6,8d#6,16p,16a#,8f#,8d#,16p,16a#,2g",
    "Happy Birthday": "birthday:d=4,o=5,b=125:8c,8c,8d,8c,8f,2e,8c,8c,8d,8c,8g,2f,8c,8c,8c6,8a,8f,8e,8d,8a#,8a#,8a,8f,8g,2f",
    "Jingle Bells": "jingle:d=4,o=5,b=125:8e,8e,4e,8e,8e,4e,8e,8g,8c,8d,2e,8f,8f,8f,8f,8f,8e,8e,8e,16e,16e,8e,8d,8d,8e,2d,4g"
  };

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
                min="40"
                max="5000"
                step="1"
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
                <span>40Hz</span>
                <span>5000Hz</span>
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
              <select
                onChange={(e) => {
                  const selectedTune = e.currentTarget.value;
                  if (selectedTune && rtttlTunes[selectedTune as keyof typeof rtttlTunes]) {
                    setRtttl(rtttlTunes[selectedTune as keyof typeof rtttlTunes]);
                  }
                }}
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
                <option value="">Load Preset Tune</option>
                <For each={Object.keys(rtttlTunes)}>
                  {(tuneName) => (
                    <option value={tuneName}>
                      {tuneName}
                    </option>
                  )}
                </For>
              </select>
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
