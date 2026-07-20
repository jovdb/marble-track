# Wheel Device

> **Agent maintenance note:** This file is the authoritative description of the Wheel device.  
> Whenever `Wheel.h`, `Wheel.cpp`, or the JSON config schema changes, update this file to match.  
> Keep every section (config, state, actions, children, JSON example) consistent with the code.

## Purpose

The Wheel device controls a rotating disc/divider wheel driven by a stepper motor.  
A zero (home) sensor provides absolute position reference so the wheel can navigate to named angles called **breakpoints**.  
On a marble track, breakpoints typically correspond to output chutes that guide marbles to different paths.

The wheel always rotates in the configured direction (never backwards during normal operation).

---

## Class hierarchy

```
Device
└── Wheel (type id: "wheel")
    ├── ConfigMixin<Wheel, WheelConfig>
    ├── StateMixin<Wheel, WheelState>
    ├── ControllableMixin<Wheel>
    └── SerializableMixin<Wheel>
```

**Files:**
- `esp32_ws/include/devices/Wheel.h`
- `esp32_ws/src/devices/Wheel.cpp`

---

## Children (created in constructor, owned by Wheel)

| Child id suffix | Type      | Role                                                    |
|-----------------|-----------|---------------------------------------------------------|
| `{id}-stepper`  | `Stepper` | Drives the stepper motor.                               |
| `{id}-zero-sensor` | `Button` | Optical/magnetic sensor that fires once per revolution at the home position. |

Children are created with hard-coded defaults and then overwritten by `jsonToConfig` when the device tree is loaded from `config.json`.

---

## Configuration (`WheelConfig`)

Persisted under the `"config"` key of the device entry in `config.json`.

| JSON key               | C++ field               | Type           | Default | Description                                                                 |
|------------------------|-------------------------|----------------|---------|-----------------------------------------------------------------------------|
| `name`                 | `name`                  | `String`       | `"Wheel"` | Human-readable device label.                                              |
| `stepsPerRevolution`   | `stepsPerRevolution`    | `long`         | `0`     | Steps per full mechanical revolution. `0` means not yet calibrated.         |
| `maxStepsPerRevolution`| `maxStepsPerRevolution` | `long`         | `10000` | Safety limit: maximum steps driven during calibration or init.              |
| `zeroPointDegree`      | `zeroPointDegree`       | `float`        | `0.0`   | Angular offset (degrees) applied when the zero sensor fires.                |
| `direction`            | `direction`             | `int`          | `1`     | Motor rotation direction: `1` = CW, `-1` = CCW.                            |
| `breakPoints`          | `breakPoints`           | `float[]`      | `[45, 90, 180, 270]` | Ordered list of target angles (0–359.9°) the wheel snaps to.  |

### Child stepper defaults (set in constructor)

```json
{
  "name": "Wheel Stepper",
  "stepperType": "DRIVER",
  "maxSpeed": 3000,
  "maxAcceleration": 3000,
  "defaultSpeed": 1000,
  "defaultAcceleration": 200
}
```

Pins are always configured by the parent device (e.g. `MarbleController`) via the serialized child config in `config.json`.

### Child zero-sensor defaults (set in constructor)

```json
{
  "name": "Wheel Zero Sensor",
  "pinMode": "pullup",
  "debounceMs": 50,
  "buttonType": "NormalOpen"
}
```

---

## State (`WheelState`)

Emitted by `addStateToJson`. All fields are read-only from the outside; they change as the device progresses through its state machine.

| JSON key                | Type     | Description                                                            |
|-------------------------|----------|------------------------------------------------------------------------|
| `state`                 | `string` | Current machine state (see table below).                               |
| `errorCode`             | `int`    | Numeric error code (`WheelErrorCode` enum cast to int). `0` = none.   |
| `errorMessage`          | `string` | Human-readable error description.                                      |
| `lastZeroPosition`      | `long`   | Stepper position (steps) when the zero sensor last fired.              |
| `currentBreakpointIndex`| `int`    | Index into `breakPoints` array. `-1` if not at a breakpoint.           |
| `targetBreakpointIndex` | `int`    | Breakpoint the wheel is currently moving toward. `-1` if none.         |
| `targetAngle`           | `float`  | Angle (degrees) the current movement is heading to. `-1` if none.      |
| `currentAngle`          | `float`  | Computed current angle based on stepper position. `-1` if unknown.     |
| `onError`               | `bool`   | Pulses `true` for one loop tick when an error is first set.            |
| `breakpointChanged`     | `bool`   | Pulses `true` for one loop tick when `currentBreakpointIndex` changes. |
| `stepsInLastRevolution` | `long`   | Steps measured in the most recently completed revolution.              |

### State machine values

| State string    | Meaning                                                              |
|-----------------|----------------------------------------------------------------------|
| `"UNKNOWN"`     | Initial/reset state; position is not known.                          |
| `"CALIBRATING"` | Measuring steps per revolution by driving through two zero crossings.|
| `"INIT"`        | Driving to find the zero sensor to establish the home position.      |
| `"MOVING"`      | Moving to an angle or breakpoint.                                    |
| `"IDLE"`        | At rest in a known position.                                         |
| `"ERROR"`       | An unrecoverable error occurred; manual reset required.              |

### Error codes (`WheelErrorCode`)

| Int value | Name                              | Description                                                 |
|-----------|-----------------------------------|-------------------------------------------------------------|
| `0`       | `None`                            | No error.                                                   |
| `1`       | `CalibrationZeroNotFound`         | Zero sensor never triggered during calibration pass 1.      |
| `2`       | `CalibrationSecondZeroNotFound`   | Zero sensor never triggered during calibration pass 2.      |
| `3`       | `ZeroNotFound`                    | Zero sensor not triggered within `maxStepsPerRevolution` during normal movement. |
| `4`       | `UnexpectedZeroTrigger`           | Steps in last revolution deviate more than 0.1 % from `stepsPerRevolution`. |

---

## Control actions

Sent via `ControllableMixin::control(action, args)` or as a WebSocket message to the device.

| Action          | Args (JSON)                            | Description                                                                    |
|-----------------|----------------------------------------|--------------------------------------------------------------------------------|
| `calibrate`     | `{ "maxStepsPerRevolution"?: long }`   | Rotates 2× `maxStepsPerRevolution` steps and measures steps between two zero crossings. Result is stored in `stepsPerRevolution`. |
| `init`          | `{ "maxStepsPerRevolution"?: long }`   | Drives toward zero sensor, then moves to the first breakpoint.                 |
| `next-breakpoint` | *(none)*                             | Advances to the next breakpoint in `breakPoints`, wrapping around.             |
| `move-to-angle` | `{ "angle": float }`                   | Moves forward to the specified angle (0–359.9°).                               |
| `stop`          | *(none)*                               | Immediately stops the stepper motor.                                           |

All actions return `false` when they cannot be executed in the current state.

### Typical workflow

```
calibrate  →  CALIBRATING  (two zero crossings measured)
           →  IDLE          (stepsPerRevolution updated)

init       →  INIT          (find zero sensor)
           →  MOVING        (move to breakPoints[0])
           →  IDLE

next-breakpoint  →  MOVING  →  IDLE  (currentBreakpointIndex incremented)
```

---

## `config.json` entry example

```json
{
  "id": "wheel",
  "type": "wheel",
  "children": [
    {
      "id": "wheel-stepper",
      "type": "stepper",
      "children": [],
      "config": {
        "name": "Wheel Stepper",
        "stepperType": "DRIVER",
        "usePwm": true,
        "maxSpeed": 500,
        "maxAcceleration": 300,
        "defaultSpeed": 120,
        "defaultAcceleration": 20,
        "stepPin":   { "pin": 4,  "expanderId": "" },
        "dirPin":    { "pin": 5,  "expanderId": "" },
        "pin1":      { "pin": -1, "expanderId": "" },
        "pin2":      { "pin": -1, "expanderId": "" },
        "pin3":      { "pin": -1, "expanderId": "" },
        "pin4":      { "pin": -1, "expanderId": "" },
        "enablePin": { "pin": 6,  "expanderId": "" },
        "invertEnable": true,
        "invertDirection": true
      }
    },
    {
      "id": "wheel-zero-sensor",
      "type": "button",
      "children": [],
      "config": {
        "pin": { "pin": 7, "expanderId": "" },
        "name": "Wheel Zero Sensor",
        "debounceTimeInMs": 50,
        "pinMode": "PullUp",
        "buttonType": "NormalOpen"
      }
    }
  ],
  "config": {
    "name": "Wheel",
    "stepsPerRevolution": 6750,
    "maxStepsPerRevolution": 6800,
    "zeroPointDegree": 240,
    "direction": 1,
    "breakPoints": [190]
  }
}
```

---

## Implementation notes

- **Always moves forward:** `moveToAngle` computes the shortest forward delta so the wheel never reverses.  
- **Position tracking during movement:** While `MOVING`, each rising edge of the zero sensor.  
- **Calibration vs init:** `calibrate` *measures* `stepsPerRevolution`; `init` *uses* the existing value to find home. Run calibrate first on a new installation.  
- **`zeroPointDegree`:** Shifts the logical zero so breakpoints are defined relative to a physical landmark, not the sensor mounting position.  
- **Registration in DeviceManager:** The `"wheel"` type string is used in `DeviceManager::loadDevicesFromJsonFile()` to construct Wheel instances. Ensure the type string in `config.json` matches.
