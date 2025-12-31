# Timer ESPHome External Component

This is an external ESPHome component that provides a reusable timer state machine.
It is designed to be independent of any alarm output (buzzer, I2S audio, light, relay, etc.).
You can attach your own alarm behavior using the exposed triggers and optional sensors.

## Features

- Start, pause, resume, cancel actions.
- Countdown with overdue detection.
- Optional Home Assistant sync inputs (state + remaining seconds sensors).
- Triggers for `started`, `paused`, `resumed`, `cancelled`, `finished`, and `tick`.
- Optional sensors for `state`, `remaining_seconds`, `set_seconds`, `running`, `paused`, and `overdue`.

## Folder Layout

```
external_components/timer/
  esphome/components/timer/
    __init__.py
    timer.h
    timer.cpp
```

## Usage (Example)

Add the external component to your ESPHome YAML:

```yaml
external_components:
  - source:
      type: local
      path: /Users/chof/development/esphome/external_components/timer/esphome/components
    refresh: 0s
```

Then configure the component:

```yaml
timer:
  id: kitchentimer
  tick_interval: 1s
  sync_interval: 5s
  max_duration: 7200s
  enable_ha_sync: true
  ha_state_sensor: timer_0_state
  ha_remaining_sensor: ha_kuchentimer_remaining

  state:
    name: "Timer State"
  remaining_seconds:
    name: "Timer Remaining"
  set_seconds:
    name: "Timer Set"
  running:
    name: "Timer Running"
  paused:
    name: "Timer Paused"
  overdue:
    name: "Timer Overdue"

  on_finished:
    then:
      - logger.log: "Timer finished"
```

Trigger actions from UI or scripts:

```yaml
script:
  - id: timer_start
    then:
      - timer.start:
          id: kitchentimer
          seconds: 300
```

## Notes

- Alarm behavior is intentionally external; use triggers to play sounds or toggle outputs.
- HA sync uses the provided HA state and remaining sensors; it does not call HA services unless you add them.
- The component clamps `set_seconds` and `remaining_seconds` to `max_duration`.

## Development

This component is designed to live outside the main ESPHome config repo and be referenced
with a local `external_components` path during development.
