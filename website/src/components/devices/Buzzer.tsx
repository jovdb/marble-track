import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { createSignal, For } from "solid-js";
import styles from "./Device.module.css";

interface IBuzzerState {
  playing: boolean;
  currentTune: string;
  pin: number;
  name: string;
  type: string;
}

export function Buzzer(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IBuzzerState>(props.id);

  const [frequency, setFrequency] = createSignal(440);
  const [rtttl, setRtttl] = createSignal(
    "Pacman:d=4,o=5,b=112:32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32c6,32p,32c7,32p,32g6,32p,32e6,32p,32c7,32g6,16p,16e6,16p,32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32d#6,32e6,32f6,32p,32f6,32f#6,32g6,32p,32g6,32g#6,32a6,32p,32b.6"
  );

  const rtttlTunes = {
    "Tetris": "tetris:d=4,o=5,b=160:e6,8b,8c6,8d6,16e6,16d6,8c6,8b,a,8a,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,2a,8p,d6,8f6,a6,8g6,8f6,e6,8e6,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,a",
    "Nokia": "nokia:d=4,o=5,b=125:8e6,8d6,8f#,8g#,8c#6,8b,8d,8e,8b,8a,8c#,8e,2a",
    "Super Mario": "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p",
    "Star Wars": "starwars:d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6",
    "Happy Birthday": "birthday:d=4,o=5,b=125:8c,8c,8d,8c,8f,2e,8c,8c,8d,8c,8g,2f,8c,8c,8c6,8a,8f,8e,8d,8a#,8a#,8a,8f,8g,2f"
  };

  const playTone = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "tone",
        frequency: frequency(),
        duration: 1000,
      })
    );
  };

  const playMelody = () => {
    if (rtttl().trim().length === 0) return;
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
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>
          🔊 {deviceState()?.name || props.id}
        </h3>
        <span class={styles["device__type-badge"]}>BUZZER</span>
      </div>
      
      <div class={styles.device__content}>
        {disabled() && (
          <div class={styles.device__error}>
            {error() || `Connection ${connectedState()}`}
          </div>
        )}
        
        {!disabled() && (
          <>
            <div class={styles.device__status}>
              <div class={`${styles["device__status-indicator"]} ${
                deviceState()?.playing 
                  ? styles["device__status-indicator--on"] 
                  : styles["device__status-indicator--off"]
              }`}></div>
              <span class={styles["device__status-text"]}>
                {deviceState()?.playing ? "Playing" : "Idle"}
                {deviceState()?.currentTune && deviceState()?.playing && " (Melody)"}
              </span>
            </div>

            <div class={styles["device__input-group"]}>
              <label class={styles.device__label} for={`freq-${props.id}`}>
                Tone Frequency: {frequency()}Hz
              </label>
              <input
                id={`freq-${props.id}`}
                class={styles.device__input}
                type="range"
                min="40"
                max="5000"
                step="1"
                value={frequency()}
                onInput={(e) => setFrequency(Number(e.currentTarget.value))}
              />
              <button 
                class={styles.device__button}
                onClick={playTone}
                disabled={disabled() || deviceState()?.playing}
              >
                Play Tone (1s)
              </button>
            </div>

            <div class={styles["device__input-group"]}>
              <label class={styles.device__label} for={`rtttl-${props.id}`}>
                RTTTL Melody:
              </label>
              <textarea
                id={`rtttl-${props.id}`}
                class={styles.device__input}
                value={rtttl()}
                onInput={(e) => setRtttl(e.currentTarget.value)}
                placeholder="Enter RTTTL string..."
                rows="3"
                style={{ 
                  "font-family": "var(--font-family-mono)",
                  "font-size": "var(--font-size-xs)",
                  "resize": "vertical",
                  "min-height": "60px"
                }}
              />
              <div class={styles.device__controls}>
                <button 
                  class={styles.device__button}
                  onClick={playMelody}
                  disabled={disabled() || deviceState()?.playing || rtttl().trim().length === 0}
                >
                  Play Melody
                </button>
                <select
                  class={styles.device__select}
                  onChange={(e) => {
                    const selectedTune = e.currentTarget.value;
                    if (selectedTune && rtttlTunes[selectedTune as keyof typeof rtttlTunes]) {
                      setRtttl(rtttlTunes[selectedTune as keyof typeof rtttlTunes]);
                    }
                  }}
                  style={{ flex: "1" }}
                >
                  <option value="">Load Preset Tune</option>
                  <For each={Object.keys(rtttlTunes)}>
                    {(tuneName) => <option value={tuneName}>{tuneName}</option>}
                  </For>
                </select>
              </div>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
