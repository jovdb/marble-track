import { type Component, type JSX } from "solid-js";
import styles from "./Popup.module.css";

interface PopupHeaderProps {
  title: string;
  children?: JSX.Element;
  class?: string;
}

/**
 * PopupHeader component for structured popup layouts.
 * Provides consistent styling for popup headers with proper spacing and borders.
 *
 * @example
 * ```tsx
 * <Popup isOpen={isOpen()}>
 *   <PopupHeader title="Modal Title" />
 *   <PopupContent>...</PopupContent>
 *   <PopupFooter>...</PopupFooter>
 * </Popup>
 *
 * // With additional children (like close button)
 * <Popup isOpen={isOpen()}>
 *   <PopupHeader title="Modal Title">
 *     <button onClick={() => setIsOpen(false)}>×</button>
 *   </PopupHeader>
 *   <PopupContent>...</PopupContent>
 *   <PopupFooter>...</PopupFooter>
 * </Popup>
 * ```
 */
const PopupHeader: Component<PopupHeaderProps> = (props) => {
  return (
    <div class={`${styles.popup__header} ${props.class || ""}`}>
      <h2 class={styles.popup__title}>{props.title}</h2>
      {props.children}
    </div>
  );
};

export default PopupHeader;
