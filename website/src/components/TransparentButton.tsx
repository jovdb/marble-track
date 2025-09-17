import "./TransparentButton.css";

/** A standard button with a transparent background and no border */
export function TransparentButton(props: any) {
  return (
    <button
      {...props}
      type="button"
      class={`transparent-button ${props.className} ${props.noHover ? "transparent-button--no-hover" : ""}`}
    >
      {props.children}
    </button>
  );
}
