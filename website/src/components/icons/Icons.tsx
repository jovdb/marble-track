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
import dotsIcon from "../../assets/icons/dots.png";
import connectedIcon from "../../assets/icons/connected.svg";
import disconnectedIcon from "../../assets/icons/disconnected.svg";

// Icon component props
export interface IconProps {
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

export const DotsIcon = (props: IconProps) => (
  <img
    src={dotsIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "WebSocket"}
  />
);

export function ConnectedIcon(props: IconProps) {
  return (
    <svg
      width={props.width || 24}
      height={props.height || 24}
      class={props.class}
      style={props.style}
      viewBox="0 0 24 24"
      fill="none"
      stroke="#000000"
      stroke-width="1"
      stroke-linecap="round"
      stroke-linejoin="round"
    >
      <path d="M18.364 19.364a9 9 0 1 0 -12.728 0" />
      <path d="M15.536 16.536a5 5 0 1 0 -7.072 0" />
      <path d="M12 13m-1 0a1 1 0 1 0 2 0a1 1 0 1 0 -2 0" />
    </svg>
  );
}

export function DisconnectedIcon(props: IconProps) {
  return (
    <svg
      width={props.width || 24}
      height={props.height || 24}
      class={props.class}
      style={props.style}
      viewBox="0 0 24 24"
      fill="none"
      stroke="#000000"
      stroke-width="1"
      stroke-linecap="round"
      stroke-linejoin="round"
    >
      <path d="M18.364 19.364a9 9 0 0 0 -9.721 -14.717m-2.488 1.509a9 9 0 0 0 -.519 13.208" />
      <path d="M15.536 16.536a5 5 0 0 0 -3.536 -8.536m-3 1a5 5 0 0 0 -.535 7.536" />
      <path d="M12 12a1 1 0 1 0 1 1" />
      <path d="M3 3l18 18" />
    </svg>
  );
}

export const RestartIcon = (props: IconProps) => (
  <svg
    width={props.width || 24}
    height={props.height || 24}
    viewBox="0 0 24 24"
    fill="none"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <path
      d="M18.364 8.05026L17.6569 7.34315C14.5327 4.21896 9.46734 4.21896 6.34315 7.34315C3.21895 10.4673 3.21895 15.5327 6.34315 18.6569C9.46734 21.7811 14.5327 21.7811 17.6569 18.6569C19.4737 16.84 20.234 14.3668 19.9377 12.0005M18.364 8.05026H14.1213M18.364 8.05026V3.80762"
      stroke="#1C274C"
      stroke-width="1.5"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
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
    case "PWMMOTOR":
      return <StepperIcon {...props} />; // Using stepper icon for PWM motor
    default:
      return <LedIcon {...props} />; // Default fallback
  }
};
