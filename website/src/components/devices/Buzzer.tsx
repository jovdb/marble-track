import { For, createMemo, createSignal } from "solid-js";
import { BuzzerIcon } from "../icons/Icons";
import deviceStyles from "./Device.module.css";
import { useBuzzer } from "../../stores/Buzzer";
import { Device } from "./Device";
import BuzzerConfig from "./BuzzerConfig";

export function Buzzer(props: { id: string }) {
  const buzzerStore = useBuzzer(props.id);
  const device = () => buzzerStore[0];
  const actions = buzzerStore[1];

  const deviceState = createMemo(() => device()?.state);
  const isPlaying = createMemo(
    () => deviceState()?.mode === "TONE" || deviceState()?.mode === "TUNE"
  );

  const [frequency, setFrequency] = createSignal(440);
  const [rtttl, setRtttl] = createSignal(
    "Pacman:d=4,o=5,b=112:32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32c6,32p,32c7,32p,32g6,32p,32e6,32p,32c7,32g6,16p,16e6,16p,32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32d#6,32e6,32f6,32p,32f6,32f#6,32g6,32p,32g6,32g#6,32a6,32p,32b.6"
  );

  const rtttlTunes = {
    Tetris:
      "tetris:d=4,o=5,b=160:e6,8b,8c6,8d6,16e6,16d6,8c6,8b,a,8a,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,2a,8p,d6,8f6,a6,8g6,8f6,e6,8e6,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,a",
    Nokia: "nokia:d=4,o=5,b=125:8e6,8d6,8f#,8g#,8c#6,8b,8d,8e,8b,8a,8c#,8e,2a",
    "Super Mario":
      "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p",
    "Star Wars":
      "starwars:d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6",
    "Tubular Bells":
      "Bells:d=4,o=5,b=280:c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6,c6,g6,c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6,c6,g6,c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6,c6,g6,c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6",
    Axel:
      "Axel:d=8,o=5,b=125:16g,16g,a#.,16g,16p,16g,c6,g,f,4g,d.6,16g,16p,16g,d#6,d6,a#,g,d6,g6,16g,16f,16p,16f,d,a#,2g,4p,16f6,d6,c6,a#,4g,a#.,16g,16p,16g,c6,g,f,4g,d.6,16g,16p,16g,d#6,d6,a#,g,d6,g6,16g,16f,16p,16f,d,a#,2g",
    "Adams Family":
      "Adams Family:d=8,o=5,b=160:c,4f,a,4f,c,4b4,2g,f,4e,g,4e,g4,4c,2f,c,4f,a,4f,c,4b4,2g,f,4e,c,4d,e,1f,c,d,e,f,1p,d,e,f#,g,1p,d,e,f#,g,4p,d,e,f#,g,4p,c,d,e,f",
    Entertainer:
      "Entertainer:d=8,o=5,b=140:d,d#,e,4c6,e,4c6,e,2c.6,c6,d6,d#6,e6,c6,d6,4e6,b,4d6,2c6,4p,d,d#,e,4c6,e,4c6,e,2c.6,p,a,g,f#,a,c6,4e6,d6,c6,a,2d6",
    "Knight Rider":
      "Knight Rider:d=32,o=5,b=63:16e,f,e,8b,16e6,f6,e6,8b,16e,f,e,16b,16e6,4d6,8p,4p,16e,f,e,8b,16e6,f6,e6,8b,16e,f,e,16b,16e6,4f6",
  };

  const playTone = () => {
    actions.tone({
      frequency: frequency(),
      duration: 1000,
    });
  };

  const playTune = () => {
    if (rtttl().trim().length === 0) return;
    actions.tune(rtttl());
  };

  const stopPlayback = () => {
    actions.stop();
  };

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <BuzzerConfig id={props.id} onClose={onClose} />}
      icon={<BuzzerIcon />}
    >
      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`freq-${props.id}`}>
          Tone Frequency: {frequency()}Hz
        </label>
        <input
          id={`freq-${props.id}`}
          class={deviceStyles.device__input}
          type="range"
          min="40"
          max="5000"
          step="1"
          value={frequency()}
          onInput={(e) => setFrequency(Number(e.currentTarget.value))}
        />
        <div class={deviceStyles.device__controls}>
          <button
            class={deviceStyles.device__button}
            onClick={isPlaying() ? stopPlayback : playTone}
          >
            {isPlaying() ? "Stop" : "Play Tone"}
          </button>
        </div>
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`rtttl-${props.id}`}>
          RTTTL Melody:
        </label>
        <textarea
          id={`rtttl-${props.id}`}
          class={deviceStyles.device__input}
          value={rtttl()}
          onInput={(e) => setRtttl(e.currentTarget.value)}
          placeholder="Enter RTTTL string..."
          rows="3"
          style={{
            "font-family": "var(--font-family-mono)",
            "font-size": "var(--font-size-xs)",
            resize: "vertical",
            "min-height": "60px",
          }}
        />
        <div class={deviceStyles.device__controls}>
          <button
            class={deviceStyles.device__button}
            onClick={isPlaying() ? stopPlayback : playTune}
            disabled={!isPlaying() && rtttl().trim().length === 0}
          >
            {isPlaying() ? "Stop" : "Play Tune"}
          </button>
          <select
            class={deviceStyles.device__select}
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
    </Device>
  );
}
