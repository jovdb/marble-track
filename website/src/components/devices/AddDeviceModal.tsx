import { createSignal, For } from "solid-js";
import { useWebSocket2 } from "../../hooks/useWebSocket2";
import { IWsSendAddDeviceMessage } from "../../interfaces/WebSockets";

interface AddDeviceModalProps {
  isOpen: boolean;
  onClose: () => void;
  onDeviceAdded: () => void;
}

const deviceTypes = [
  { value: "led", label: "LED", description: "Light Emitting Diode" },
  { value: "button", label: "Button", description: "Push Button Input" },
  { value: "servo", label: "Servo", description: "Servo Motor" },
  { value: "stepper", label: "Stepper", description: "Stepper Motor" },
  { value: "buzzer", label: "Buzzer", description: "Sound Buzzer" },
  { value: "pwmmotor", label: "PWM Motor", description: "PWM Controlled Motor" },
];

export default function AddDeviceModal(props: AddDeviceModalProps) {
  const [deviceType, setDeviceType] = createSignal("led");
  const [deviceId, setDeviceId] = createSignal("");
  const [isLoading, setIsLoading] = createSignal(false);
  const [error, setError] = createSignal("");
  
  const [, { sendMessage }] = useWebSocket2();

  const handleSubmit = async (e: Event) => {
    e.preventDefault();
    
    if (!deviceId().trim()) {
      setError("Device ID is required");
      return;
    }

    setIsLoading(true);
    setError("");

    try {
      const message: IWsSendAddDeviceMessage = {
        type: "add-device",
        deviceType: deviceType(),
        deviceId: deviceId().trim(),
        config: {}
      };

      sendMessage(message);
      
      // Close modal and reset form
      props.onClose();
      setDeviceType("led");
      setDeviceId("");
      props.onDeviceAdded();
      
    } catch {
      setError("Failed to add device");
    } finally {
      setIsLoading(false);
    }
  };

  const handleClose = () => {
    if (!isLoading()) {
      setDeviceType("led");
      setDeviceId("");
      setError("");
      props.onClose();
    }
  };

  if (!props.isOpen) return null;

  return (
    <div class="modal-backdrop" onClick={handleClose}>
      <dialog open class="modal-dialog" onClick={(e) => e.stopPropagation()}>
        <article>
          <header>
            <h3>Add New Device</h3>
            <button
              type="button"
              class="close"
              aria-label="Close"
              onClick={handleClose}
              disabled={isLoading()}
            >
              ×
            </button>
          </header>
          
          <form onSubmit={handleSubmit}>
            <fieldset disabled={isLoading()}>
              <legend>Device Configuration</legend>
              
              <label>
                Device Type
                <select
                  value={deviceType()}
                  onInput={(e) => setDeviceType(e.currentTarget.value)}
                  required
                >
                  <For each={deviceTypes}>
                    {(type) => (
                      <option value={type.value}>
                        {type.label} - {type.description}
                      </option>
                    )}
                  </For>
                </select>
              </label>

              <label>
                Device ID
                <input
                  type="text"
                  value={deviceId()}
                  onInput={(e) => setDeviceId(e.currentTarget.value)}
                  placeholder="Enter unique device ID (e.g., led1, button2)"
                  required
                  pattern="[a-zA-Z0-9_-]+"
                  title="Only letters, numbers, underscores, and hyphens allowed"
                />
                <small>Use only letters, numbers, underscores, and hyphens</small>
              </label>

              {error() && (
                <div role="alert" style="color: var(--color-danger-600); margin-top: 0.5rem;">
                  {error()}
                </div>
              )}
            </fieldset>
          </form>

          <footer>
            <button
              type="button"
              onClick={handleClose}
              disabled={isLoading()}
              class="secondary"
            >
              Cancel
            </button>
            <button
              type="submit"
              onClick={handleSubmit}
              disabled={isLoading() || !deviceId().trim()}
              aria-busy={isLoading()}
            >
              {isLoading() ? "Adding..." : "Add Device"}
            </button>
          </footer>
        </article>
      </dialog>
    </div>
  );
}