#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <string>
#include <vector>

namespace esphome {
namespace timer_ext {

enum class TimerState : uint8_t {
  STOPPED = 0,
  RUNNING = 1,
  PAUSED = 2,
};

class TimerStartedTrigger;
class TimerPausedTrigger;
class TimerResumedTrigger;
class TimerCancelledTrigger;
class TimerFinishedTrigger;
class TimerTickTrigger;

class TimerComponent : public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_tick_interval_ms(uint32_t value) { this->tick_interval_ms_ = value; }
  void set_sync_interval_ms(uint32_t value) { this->sync_interval_ms_ = value; }
  void set_max_duration_seconds(uint32_t value);
  void set_initial_set_seconds(uint32_t value) { this->set_seconds_ = this->clamp_seconds_(value); }
  void set_enable_ha_sync(bool value) { this->enable_ha_sync_ = value; }

  void set_ha_state_sensor(text_sensor::TextSensor *sensor) { this->ha_state_sensor_ = sensor; }
  void set_ha_remaining_sensor(sensor::Sensor *sensor) { this->ha_remaining_sensor_ = sensor; }

  void set_remaining_seconds_sensor(sensor::Sensor *sensor) { this->remaining_seconds_sensor_ = sensor; }
  void set_set_seconds_sensor(sensor::Sensor *sensor) { this->set_seconds_sensor_ = sensor; }
  void set_state_text_sensor(text_sensor::TextSensor *sensor) { this->state_text_sensor_ = sensor; }
  void set_running_binary_sensor(binary_sensor::BinarySensor *sensor) { this->running_binary_sensor_ = sensor; }
  void set_paused_binary_sensor(binary_sensor::BinarySensor *sensor) { this->paused_binary_sensor_ = sensor; }
  void set_overdue_binary_sensor(binary_sensor::BinarySensor *sensor) { this->overdue_binary_sensor_ = sensor; }

  void register_started_trigger(TimerStartedTrigger *trigger) { this->started_triggers_.push_back(trigger); }
  void register_paused_trigger(TimerPausedTrigger *trigger) { this->paused_triggers_.push_back(trigger); }
  void register_resumed_trigger(TimerResumedTrigger *trigger) { this->resumed_triggers_.push_back(trigger); }
  void register_cancelled_trigger(TimerCancelledTrigger *trigger) { this->cancelled_triggers_.push_back(trigger); }
  void register_finished_trigger(TimerFinishedTrigger *trigger) { this->finished_triggers_.push_back(trigger); }
  void register_tick_trigger(TimerTickTrigger *trigger) { this->tick_triggers_.push_back(trigger); }

  void set_seconds(int seconds);
  void start();
  void start(int seconds);
  void pause(bool from_ha = false);
  void resume(bool from_ha = false);
  void cancel(bool from_ha = false);

  int get_remaining_seconds() const { return this->remaining_seconds_; }
  int get_set_seconds() const { return this->set_seconds_; }
  bool is_overdue() const { return this->overdue_; }
  TimerState get_state() const { return this->state_; }

 protected:
  void start_(int seconds, bool from_ha);
  void tick_();
  void publish_state_();
  const char *state_to_string_() const;
  int clamp_seconds_(int seconds) const;
  void clamp_set_and_remaining_();
  void handle_ha_state_(const std::string &state);
  void handle_ha_remaining_(float value);
  void mark_synced_from_ha_();

  TimerState state_{TimerState::STOPPED};
  int set_seconds_{0};
  int remaining_seconds_{0};
  bool overdue_{false};
  bool synched_{false};
  bool enable_ha_sync_{true};
  uint32_t tick_interval_ms_{1000};
  uint32_t sync_interval_ms_{5000};
  uint32_t max_duration_seconds_{7200};
  uint32_t last_ha_sync_ms_{0};

  text_sensor::TextSensor *ha_state_sensor_{nullptr};
  sensor::Sensor *ha_remaining_sensor_{nullptr};

  sensor::Sensor *remaining_seconds_sensor_{nullptr};
  sensor::Sensor *set_seconds_sensor_{nullptr};
  text_sensor::TextSensor *state_text_sensor_{nullptr};
  binary_sensor::BinarySensor *running_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *paused_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *overdue_binary_sensor_{nullptr};

  std::vector<TimerStartedTrigger *> started_triggers_;
  std::vector<TimerPausedTrigger *> paused_triggers_;
  std::vector<TimerResumedTrigger *> resumed_triggers_;
  std::vector<TimerCancelledTrigger *> cancelled_triggers_;
  std::vector<TimerFinishedTrigger *> finished_triggers_;
  std::vector<TimerTickTrigger *> tick_triggers_;
};

class TimerStartedTrigger : public Trigger<bool> {
 public:
  explicit TimerStartedTrigger(TimerComponent *parent) { parent->register_started_trigger(this); }
};

class TimerPausedTrigger : public Trigger<bool> {
 public:
  explicit TimerPausedTrigger(TimerComponent *parent) { parent->register_paused_trigger(this); }
};

class TimerResumedTrigger : public Trigger<bool> {
 public:
  explicit TimerResumedTrigger(TimerComponent *parent) { parent->register_resumed_trigger(this); }
};

class TimerCancelledTrigger : public Trigger<bool> {
 public:
  explicit TimerCancelledTrigger(TimerComponent *parent) { parent->register_cancelled_trigger(this); }
};

class TimerFinishedTrigger : public Trigger<bool> {
 public:
  explicit TimerFinishedTrigger(TimerComponent *parent) { parent->register_finished_trigger(this); }
};

class TimerTickTrigger : public Trigger<int> {
 public:
  explicit TimerTickTrigger(TimerComponent *parent) { parent->register_tick_trigger(this); }
};

template<typename... Ts> class TimerStartAction : public Action<Ts...> {
 public:
  explicit TimerStartAction(TimerComponent *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(int, seconds)

  void play(const Ts &... x) override {
    if (this->seconds_.has_value()) {
      this->parent_->start(this->seconds_.value(x...));
      return;
    }
    this->parent_->start();
  }

 protected:
  TimerComponent *parent_;
};

template<typename... Ts> class TimerPauseAction : public Action<Ts...> {
 public:
  explicit TimerPauseAction(TimerComponent *parent) : parent_(parent) {}
  void play(const Ts &... x) override { this->parent_->pause(); }

 protected:
  TimerComponent *parent_;
};

template<typename... Ts> class TimerResumeAction : public Action<Ts...> {
 public:
  explicit TimerResumeAction(TimerComponent *parent) : parent_(parent) {}
  void play(const Ts &... x) override { this->parent_->resume(); }

 protected:
  TimerComponent *parent_;
};

template<typename... Ts> class TimerCancelAction : public Action<Ts...> {
 public:
  explicit TimerCancelAction(TimerComponent *parent) : parent_(parent) {}
  void play(const Ts &... x) override { this->parent_->cancel(); }

 protected:
  TimerComponent *parent_;
};

template<typename... Ts> class TimerSetSecondsAction : public Action<Ts...> {
 public:
  explicit TimerSetSecondsAction(TimerComponent *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(int, seconds)

  void play(const Ts &... x) override {
    if (!this->seconds_.has_value()) {
      return;
    }
    this->parent_->set_seconds(this->seconds_.value(x...));
  }

 protected:
  TimerComponent *parent_;
};

}  // namespace timer_ext
}  // namespace esphome
