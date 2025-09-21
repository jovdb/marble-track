import { type Component, type JSX } from "solid-js";
import { Portal } from "solid-js/web";
import { Show } from "solid-js";
import styles from "./Popup.module.css";

interface PopupProps {
  isOpen: boolean;
  children: JSX.Element;
}

/**
 * Popup component that renders content in a portal overlay.
 * Use with PopupHeader, PopupContent, and PopupFooter for structured layouts.
 *
 * @example
 * ```tsx
 * // Simple popup
 * <Popup isOpen={isOpen()}>
 *   <p>This is a simple popup.</p>
 * </Popup>
 *
 * // Structured popup with header, content, and footer
 * <Popup isOpen={isOpen()}>
 *   <PopupHeader>
 *     <h2>Title</h2>
 *   </PopupHeader>
 *   <PopupContent>
 *     <p>Content goes here.</p>
 *   </PopupContent>
 *   <PopupFooter>
 *     <button onClick={() => setIsOpen(false)}>Close</button>
 *   </PopupFooter>
 * </Popup>
 * ```
 */
const Popup: Component<PopupProps> = (props) => {
  return (
    <Portal mount={document.body}>
      <Show when={props.isOpen}>
        <div class={styles.popup}>
          <div class={styles.popup__content}>{props.children}</div>
        </div>
      </Show>
    </Portal>
  );
};

export { Popup };
