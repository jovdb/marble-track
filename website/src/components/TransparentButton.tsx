import { ComponentProps } from "solid-js";
import "./TransparentButton.css";

/** A standard button with a transparent background and no border */
export function TransparentButton(props: ComponentProps<"button"> & { noHover?: boolean }) {
  return (
    <button
      {...props}
      type="button"
      class={`transparent-button ${props.class ?? ""} ${props.noHover ? "transparent-button--no-hover" : ""}`}
    >
      {props.children}
    </button>
  );
}
