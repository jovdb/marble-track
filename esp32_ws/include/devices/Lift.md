# Lift Device

> **Agent maintenance note:** This file is the authoritative description of the Lift device.  
> Whenever `Lift.h`, `Lift.cpp`, or the JSON config schema changes, update this file to match.  
> Keep every section (config, state, actions, children, JSON example) consistent with the code.

## Purpose

The Lift device is a vertical marble elevator that moves marbles between a lower loading position and an upper unloading position.  
It uses a stepper motor for vertical travel, a limit switch to detect the bottom (zero) position, a ball sensor to detect a waiting marble at the bottom, and two servos (loader and unloader) to transfer marbles in and out.

---

## Class hierarchy

```
Device
└── Lift (type id: "lift")
    ├── ConfigMixin<Lift, LiftConfig>
    ├── StateMixin<Lift, LiftState>
    ├── ControllableMixin<Lift>
    └── SerializableMixin<Lift>
```

**Files:**
- `esp32_ws/include/devices/Lift.h`
- `esp32_ws/src/devices/Lift.cpp`

---

## Children (created in constructor, owned by Lift)

| Child id suffix      | Type      | Role                                                                     |
|----------------------|-----------|--------------------------------------------------------------------------|
| `{id}-stepper`       | `Stepper` | Drives the vertical stepper motor.                                       |
| `{id}-limit`         | `Button`  | Limit switch at the bottom: pressed = lift is at `minSteps` (zero).     |
| `{id}-ball-sensor`   | `Button`  | Sensor at the bottom: pressed = a marble is waiting to be loaded.        |
| `{id}-loader`        | `Servo`   | Servo gate at the bottom that lets a marble into the lift cage.          |
| `{id}-unloader`      | `Servo`   | Servo gate at the top that releases a marble from the lift cage.         |

Children are created with hard-coded defaults and then overwritten by `jsonToConfig` when the device tree is loaded from `config.json`. Pins are always provided via the serialized child config (typically set by the parent `MarbleController`).

---

## Configuration (`LiftConfig`)

Persisted under the `"config"` key of the device entry in `config.json`.

| JSON key     | C++ field    | Type    | Default  | Description                                                               |
|--------------|--------------|---------|----------|---------------------------------------------------------------------------|
| `name`       | `name`       | `String`| `"Lift"` | Human-readable device label.                                              |
| `minSteps`   | `minSteps`   | `long`  | `0`      | Stepper position (steps) at the bottom. The limit switch resets position to this value. |
| `maxSteps`   | `maxSteps`   | `long`  | `1000`   | Stepper position (steps) at the top (fully raised).                       |
| `downFactor` | `downFactor` | `float` | `1.015`  | Overshoot multiplier applied when moving down, to guarantee the limit switch is reached. |

### Child stepper defaults (set in constructor)

```json
{
  "name": "Lift Stepper",
  "stepperType": "DRIVER",
  "maxSpeed": 400,
  "maxAcceleration": 100,
  "defaultSpeed": 150,
  "defaultAcceleration": 50
}
```

### Child loader servo note

The loader servo's `defaultDurationInMs` controls how long the gate stays open during a load cycle.  
The lift waits `defaultDurationInMs + 500 ms` before calling `loadBallEnd`.

### Child unloader servo note

The unloader servo's `defaultDurationInMs` controls the open phase of the unload animation.  
The lift waits the scaled duration (adjusted by `durationRatio`) before calling `unloadBallEnd`.

---

## State (`LiftState`)

Emitted by `addStateToJson`. All fields are read-only from the outside.

| JSON key            | Type     | Description                                                                       |
|---------------------|----------|-----------------------------------------------------------------------------------|
| `state`             | `string` | Current machine state (see table below).                                          |
| `ballWaitingSince`  | `ulong`  | `millis()` timestamp when a marble first appeared at the ball sensor. `0` if no marble is waiting. |
| `isLoaded`          | `bool`   | `true` when the lift cage currently holds a marble.                               |
| `currentPosition`   | `long`   | Stepper step count (omitted from the JSON payload while the lift is moving).      |
| `errorMessage`      | `string` | Human-readable description of the last error.                                     |
| `errorCode`         | `string` | Error code string (see table below).                                              |

### State machine values

| State string       | Meaning                                                                     |
|--------------------|-----------------------------------------------------------------------------|
| `"Unknown"`        | Initial/reset state; position is not known.                                 |
| `"Init"`           | Running the initialization (homing) sequence.                               |
| `"LiftDown"`       | At the bottom, limit switch active, ready to load.                          |
| `"LiftDownLoading"`| At the bottom, loader servo open; waiting for marble to enter.              |
| `"MovingUp"`       | Stepper is driving the cage upward toward `maxSteps`.                       |
| `"LiftUp"`         | At the top, ready to unload.                                                |
| `"LiftUpUnloading"`| At the top, unloader servo open; waiting for marble to leave.               |
| `"MovingDown"`     | Stepper is driving the cage downward; waiting for limit switch.             |
| `"Error"`          | An unrecoverable error occurred; manual reset (re-init) required.           |

### State transitions

```
init()
  └─ INIT  →  (unload end, move down, wait for limit switch)  →  LiftDown

loadBall()                           up()
  LiftDown  →  LiftDownLoading  →  LiftDown  →  MovingUp  →  LiftUp

unloadBall()                         down()
  LiftUp  →  LiftUpUnloading  →  LiftUp  →  MovingDown  →  LiftDown
```

### Error codes (`LiftErrorCode`)

| String value                | Description                                                         |
|-----------------------------|---------------------------------------------------------------------|
| `""` (empty)                | No error (`NONE`).                                                  |
| `"LIFT_INIT_NO_ZERO"`       | Limit switch not triggered during the init homing move.             |
| `"LIFT_CONFIGURATION_ERROR"`| Required pin not configured, or invalid `minSteps`/`maxSteps`.     |
| `"LIFT_STATE_ERROR"`        | An unexpected state was encountered in the state machine.           |
| `"LIFT_NO_ZERO"`            | Limit switch not triggered while moving down (stepper stopped first).|

---

## Control actions

Sent via `ControllableMixin::control(action, args)` or as a WebSocket message to the device.

| Action       | Args (JSON)                      | Description                                                                    |
|--------------|----------------------------------|--------------------------------------------------------------------------------|
| `init`       | *(none)*                         | Homes the lift: ends any unload, drives to bottom until limit switch fires, then stays at `LiftDown`. |
| `up`         | `{ "speedRatio"?: float }`       | Moves cage from `LiftDown` (or `MovingDown`/`MovingUp`) to `maxSteps`. Default `speedRatio` = 1.0. |
| `down`       | `{ "speedRatio"?: float }`       | Moves cage from `LiftUp` (or `MovingUp`/`MovingDown`) to bottom (limit switch). Applies `downFactor` overshoot. Default `speedRatio` = 1.0. |
| `loadBall`   | *(none)*                         | Opens the loader servo to admit a marble. Only valid in `LiftDown`.            |
| `unloadBall` | `{ "durationRatio"?: float }`    | Opens the unloader servo to release a marble. Only valid in `LiftUp`. Default `durationRatio` = 1.0. |

All actions return `false` when they cannot be executed in the current state.

### Typical marble-cycle workflow

```
init                           (home to bottom)
loadBall                       (open loader gate → marble enters)
up                             (raise to top)
unloadBall                     (open unloader gate → marble exits)
down                           (lower to bottom)
```

---

## `config.json` entry example

```json
{
  "id": "lift",
  "type": "lift",
  "children": [
    {
      "id": "lift-stepper",
      "type": "stepper",
      "children": [],
      "config": {
        "name": "Lift Stepper",
        "stepperType": "DRIVER",
        "usePwm": true,
        "maxSpeed": 2500,
        "maxAcceleration": 5000,
        "defaultSpeed": 750,
        "defaultAcceleration": 200,
        "stepPin":   { "pin": 1,  "expanderId": "" },
        "dirPin":    { "pin": 2,  "expanderId": "" },
        "pin1":      { "pin": -1, "expanderId": "" },
        "pin2":      { "pin": -1, "expanderId": "" },
        "pin3":      { "pin": -1, "expanderId": "" },
        "pin4":      { "pin": -1, "expanderId": "" },
        "enablePin": { "pin": 42, "expanderId": "" },
        "invertEnable": true,
        "invertDirection": false
      }
    },
    {
      "id": "lift-limit",
      "type": "button",
      "children": [],
      "config": {
        "pin": { "pin": 38, "expanderId": "" },
        "name": "Lift Limit Switch",
        "debounceTimeInMs": 50,
        "pinMode": "PullUp",
        "buttonType": "NormalOpen"
      }
    },
    {
      "id": "lift-ball-sensor",
      "type": "button",
      "children": [],
      "config": {
        "pin": { "pin": 39, "expanderId": "" },
        "name": "Lift Ball Sensor",
        "debounceTimeInMs": 50,
        "pinMode": "PullUp",
        "buttonType": "NormalOpen"
      }
    },
    {
      "id": "lift-loader",
      "type": "servo",
      "children": [],
      "config": {
        "pin": 40,
        "name": "Lift Loader",
        "mcpwmChannel": 0,
        "frequency": 50,
        "resolutionBits": 10,
        "minDutyCycle": 9.7,
        "maxDutyCycle": 4.5,
        "defaultDurationInMs": 200
      }
    },
    {
      "id": "lift-unloader",
      "type": "servo",
      "children": [],
      "config": {
        "pin": 41,
        "name": "Lift Unloader",
        "mcpwmChannel": 1,
        "frequency": 50,
        "resolutionBits": 10,
        "minDutyCycle": 11.2,
        "maxDutyCycle": 5.4,
        "defaultDurationInMs": 1200
      }
    }
  ],
  "config": {
    "name": "Lift",
    "minSteps": 0,
    "maxSteps": 4455,
    "downFactor": 1.015
  }
}
```

---

## Implementation notes

- **Homing:** The limit switch defines position `0` (`minSteps`). When the switch fires while the lift is moving down, the stepper position is immediately reset to `0` and the motor stops with high deceleration.  
- **`downFactor` overshoot:** To guarantee the limit switch is always reached, the downward move target is computed as `(minSteps - currentPos) * downFactor`. This adds a small extra travel that the limit switch intercepts.  
- **`ballWaitingSince`:** Tracks when a marble arrived at the ball sensor so consumers can implement time-based load triggers.  
- **`currentPosition` suppression:** The state JSON omits `currentPosition` while moving to avoid flooding WebSocket clients with rapid position updates.  
- **Error recovery:** Any error requires calling `init` again. There is no soft-reset action; the initialization sequence clears the error state.  
- **Registration in DeviceManager:** The `"lift"` type string is used in `DeviceManager::loadDevicesFromJsonFile()` to construct Lift instances. Ensure the type string in `config.json` matches.  
- **Child pin ownership:** The Lift constructor creates children with `-1` placeholder pins. Actual pins are written by the serialized child entries in `config.json` via `jsonToConfig`.
