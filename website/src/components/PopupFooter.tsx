import { type Component, type JSX } from "solid-js";
import styles from "./Popup.module.css";

interface PopupFooterProps {
  children: JSX.Element;
  class?: string;
}

/**
 * PopupFooter component for structured popup layouts.
 * Provides consistent styling for popup footers with action buttons.
 *
 * @example
 * ```tsx
 * <Popup isOpen={isOpen()}>
 *   <PopupHeader>...</PopupHeader>
 *   <PopupContent>...</PopupContent>
 *   <PopupFooter>
 *     <button onClick={handleCancel}>Cancel</button>
 *     <button onClick={handleConfirm}>Confirm</button>
 *   </PopupFooter>
 * </Popup>
 * ```
 */
const PopupFooter: Component<PopupFooterProps> = (props) => {
  return <div class={`${styles.popup__footer} ${props.class || ""}`}>{props.children}</div>;
};

export default PopupFooter;
