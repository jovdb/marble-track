import { type Component, type JSX } from "solid-js";
import styles from "./Popup.module.css";

interface PopupContentProps {
  children: JSX.Element;
  class?: string;
}

/**
 * PopupContent component for structured popup layouts.
 * Provides the main content area with proper scrolling and spacing.
 *
 * @example
 * ```tsx
 * <Popup isOpen={isOpen()}>
 *   <PopupHeader>...</PopupHeader>
 *   <PopupContent>
 *     <p>Main content goes here.</p>
 *     <form>...</form>
 *   </PopupContent>
 *   <PopupFooter>...</PopupFooter>
 * </Popup>
 * ```
 */
const PopupContent: Component<PopupContentProps> = (props) => {
  return (
    <div class={`${styles.popup__body} ${props.class || ""}`}>
      {props.children}
    </div>
  );
};

export default PopupContent;