// Device PNG Icons
import { JSX } from "solid-js";

// Import PNG icons
import ledIcon from "../../assets/icons/led.png";
import servoIcon from "../../assets/icons/servo.png";
import buttonIcon from "../../assets/icons/button.png";
import buzzerIcon from "../../assets/icons/buzzer1.png";
import stepperIcon from "../../assets/icons/stepper.png";
import devicesIcon from "../../assets/icons/devices.png";
import websocketIcon from "../../assets/icons/websocket.png";

interface IconProps {
  width?: number;
  height?: number;
  class?: string;
  style?: JSX.CSSProperties;
  alt?: string;
}

export const LedIcon = (props: IconProps) => (
  <img
    src={ledIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "LED"}
  />
);

export const ServoIcon = (props: IconProps) => (
  <img
    src={servoIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "Servo"}
  />
);

export const ButtonIcon = (props: IconProps) => (
  <img
    src={buttonIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "Button"}
  />
);

export const BuzzerIcon = (props: IconProps) => (
  <img
    src={buzzerIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "Buzzer"}
  />
);

export const StepperIcon = (props: IconProps) => (
  <img
    src={stepperIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "Stepper Motor"}
  />
);

export const ClipboardIcon = (props: IconProps) => (
  <img
    src={devicesIcon}
    width={props.width || 20}
    height={props.height || 20}
    class={props.class}
    style={props.style}
    alt={props.alt || "Devices"}
  />
);

export const RadioIcon = (props: IconProps) => (
  <img
    src={websocketIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "WebSocket"}
  />
);

export const MessageIcon = (props: IconProps) => (
  <svg
    width={props.width || 20}
    height={props.height || 20}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    class={props.class}
    style={props.style}
  >
    <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z" />
    <path d="M8 10h.01" />
    <path d="M12 10h.01" />
    <path d="M16 10h.01" />
  </svg>
);

// Helper function to get device icon by type
export const getDeviceIcon = (type: string, props?: IconProps) => {
  switch (type.toUpperCase()) {
    case "LED":
      return <LedIcon {...props} />;
    case "SERVO":
      return <ServoIcon {...props} />;
    case "BUTTON":
      return <ButtonIcon {...props} />;
    case "BUZZER":
      return <BuzzerIcon {...props} />;
    case "STEPPER":
      return <StepperIcon {...props} />;
    default:
      return <LedIcon {...props} />; // Default fallback
  }
};
