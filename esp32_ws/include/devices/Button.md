# Button Device

> **Agent maintenance note:** This file is the authoritative description of the Button device.  
> Whenever `Button.h`, `Button.cpp`, or the JSON config schema changes, update this file to match.  
> Keep every section (config, state, actions, children, JSON example) consistent with the code.

## Purpose

The Button device reads a single digital input pin — physical push-buttons, limit switches, reed contacts, optical interrupters, or any other binary sensor.  
It reports a debounced `isPressed` value and fires a state-change notification whenever the value changes.

The device understands two electrical wiring styles (**NormallyOpen** / **NormallyClosed**) and three pull configurations (**PullUp** / **PullDown** / **Floating**), letting it map any real-world sensor to a consistent `isPressed` semantic.

---

## Class hierarchy

```
Device
└── Button (type id: "button")
    ├── ConfigMixin<Button, ButtonConfig>
    ├── StateMixin<Button, ButtonState>
    ├── ControllableMixin<Button>
    └── SerializableMixin<Button>
```

**Files:**
- `esp32_ws/include/devices/Button.h`
- `esp32_ws/src/devices/Button.cpp`

---

## Press logic truth table

`value` is the raw integer read from the pin (0 = LOW, 1 = HIGH).

| `pinMode` | `value` | `buttonType`  | `isPressed` | Reason                                          |
|-----------|---------|---------------|-------------|------------------------------------------------|
| PullUp    | 0       | NormallyOpen  | **true**    | LOW → contact closed → NO pressed              |
| PullUp    | 1       | NormallyOpen  | **false**   | HIGH → contact open → NO not pressed           |
| PullDown  | 0       | NormallyOpen  | **false**   | LOW → contact open → NO not pressed            |
| PullDown  | 1       | NormallyOpen  | **true**    | HIGH → contact closed → NO pressed             |
| Floating  | 0       | NormallyOpen  | **false**   | LOW → no signal → not active                   |
| Floating  | 1       | NormallyOpen  | **true**    | HIGH → signal present → active                 |
| PullUp    | 0       | NormallyClosed| **false**   | LOW → contact closed → NC at rest (not pressed)|
| PullUp    | 1       | NormallyClosed| **true**    | HIGH → contact open → NC pressed               |
| PullDown  | 0       | NormallyClosed| **true**    | LOW → contact open → NC pressed                |
| PullDown  | 1       | NormallyClosed| **false**   | HIGH → contact closed → NC at rest             |
| Floating  | 0       | NormallyClosed| **false**   | No pull → NC acts as NO (no inversion)         |
| Floating  | 1       | NormallyClosed| **true**    | No pull → NC acts as NO (no inversion)         |

**Rules applied in code (`readIsButtonPressed`):**

1. `contactClosed`:  
   - PullUp → `LOW == pinState`  
   - PullDown / Floating → `HIGH == pinState`
2. NC inversion (only when `pinMode != Floating`):  
   `isPressed = (isNC && hasPull) ? !contactClosed : contactClosed`

With **Floating** mode there is no defined voltage reference, so a NormallyClosed sensor behaves identically to NormallyOpen (HIGH = active).

---

## Configuration (`ButtonConfig`)

Persisted under the `"config"` key of the device entry in `config.json`.

| JSON key          | C++ field        | Type     | Default          | Description                                                    |
|-------------------|------------------|----------|------------------|----------------------------------------------------------------|
| `name`            | `name`           | `String` | `"Button"`       | Human-readable device label.                                   |
| `pin`             | `pinConfig`      | object   | —                | Pin descriptor (see pin configuration below).                  |
| `pinMode`         | `pinMode`        | `String` | `"Floating"`     | `"PullUp"`, `"PullDown"`, or `"Floating"`.                    |
| `buttonType`      | `buttonType`     | `String` | `"NormallyOpen"` | `"NormallyOpen"` or `"NormallyClosed"`.                        |
| `debounceMs`      | `debounceTimeInMs` | `ulong`| `50`             | Debounce window in milliseconds.                               |

### Pin configuration

The `pin` field accepts either a plain GPIO number or an expander descriptor:

```json
"pin": 4
```
```json
"pin": { "expanderId": "io-expander-1", "pin": 3 }
```

---

## State (`ButtonState`)

| JSON key          | Type   | Description                                               |
|-------------------|--------|-----------------------------------------------------------|
| `isPressed`       | `bool` | Debounced pressed state after applying buttonType logic.  |
| `isPressedChanged`| `bool` | `true` for exactly one loop tick after `isPressed` changed. Reset on next tick. |
| `value`           | `int`  | Raw pin reading (0 = LOW, 1 = HIGH).                      |

---

## Actions (WebSocket control)

| Action    | Args | Description                                          |
|-----------|------|------------------------------------------------------|
| `press`   | —    | Simulates a button press (bypasses physical pin).    |
| `release` | —    | Simulates a button release.                          |

Simulation is sticky: once `press` or `release` is called, the button remains in that state until the opposite action is sent or the device is torn down.

---

## JSON example (`config.json` entry)

```json
{
  "id": "my-button",
  "type": "button",
  "config": {
    "name": "My Button",
    "pin": 15,
    "pinMode": "PullUp",
    "buttonType": "NormallyOpen",
    "debounceMs": 50
  }
}
```

---

## Typical state WebSocket message

```json
{
  "type": "state",
  "deviceId": "my-button",
  "state": {
    "isPressed": true,
    "isPressedChanged": true,
    "value": 0
  }
}
```
(`value` is 0 / LOW because PullUp + NormallyOpen: LOW = contact closed = pressed.)
