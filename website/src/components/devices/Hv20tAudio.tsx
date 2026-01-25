import { createMemo, createSignal } from "solid-js";
import { Device } from "./Device";
import deviceStyles from "./Device.module.css";
import { getDeviceIcon } from "../icons/Icons";
import { useHv20tAudio } from "../../stores/Hv20tAudio";
import Hv20tAudioConfig from "./Hv20tAudioConfig";

export function Hv20tAudio(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const hv20tStore = useHv20tAudio(props.id);
  const device = () => hv20tStore[0];
  const actions = hv20tStore[1];

  const state = createMemo(() => device()?.state);
  const volumePercent = createMemo(() => state()?.volumePercent ?? 50);
  const isBusy = createMemo(() => Boolean(state()?.isBusy));

  const [songIndex, setSongIndex] = createSignal(1);
  const [volume, setVolume] = createSignal(volumePercent());

  const handlePlay = () => actions.play(Math.trunc(songIndex()));
  const handleStop = () => actions.stop();
  const handleSetVolume = () => actions.setVolume(Math.trunc(volume()));

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <Hv20tAudioConfig id={props.id} onClose={onClose} />}
      icon={getDeviceIcon("hv20t")}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
      stateComponent={() => null}
    >
      <div class={deviceStyles.device__status}>
        <span class={deviceStyles["device__status-text"]}>
          Status: {isBusy() ? "Playing" : "Idle"}
        </span>
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`song-index-${props.id}`}>
          Song index
        </label>
        <input
          id={`song-index-${props.id}`}
          class={deviceStyles.device__input}
          type="number"
          min="0"
          max="255"
          value={songIndex()}
          onInput={(event) => setSongIndex(Number(event.currentTarget.value))}
        />
        <div class={deviceStyles.device__controls}>
          <button class={deviceStyles.device__button} onClick={handlePlay}>
            Play
          </button>
          <button class={deviceStyles.device__button} onClick={handleStop}>
            Stop
          </button>
        </div>
      </div>

      <div class={deviceStyles["device__input-group"]}>
        <label class={deviceStyles.device__label} for={`volume-${props.id}`}>
          Volume: {Math.trunc(volume())}%
        </label>
        <input
          id={`volume-${props.id}`}
          class={deviceStyles.device__input}
          type="range"
          min="0"
          max="100"
          value={volume()}
          onInput={(event) => setVolume(Number(event.currentTarget.value))}
        />
        <div class={deviceStyles.device__controls}>
          <button class={deviceStyles.device__button} onClick={handleSetVolume}>
            Set volume
          </button>
        </div>
      </div>
    </Device>
  );
}
