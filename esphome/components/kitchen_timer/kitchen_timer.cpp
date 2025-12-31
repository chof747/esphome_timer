#include "kitchen_timer.h"
#include "esphome/core/log.h"

namespace esphome {
namespace kitchen_timer {

static const char *const TAG = "kitchen_timer";

void KitchenTimerComponent::setup() {
  this->set_interval(this->tick_interval_ms_, [this]() { this->tick_(); });

  if (this->ha_state_sensor_ != nullptr) {
    this->ha_state_sensor_->add_on_state_callback(
        [this](const std::string &state) { this->handle_ha_state_(state); });
  }

  if (this->ha_remaining_sensor_ != nullptr) {
    this->ha_remaining_sensor_->add_on_state_callback(
        [this](float value) { this->handle_ha_remaining_(value); });
  }

  this->publish_state_();
}

void KitchenTimerComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Kitchen Timer:");
  ESP_LOGCONFIG(TAG, "  Tick Interval: %u ms", this->tick_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Sync Interval: %u ms", this->sync_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Max Duration: %u s", this->max_duration_seconds_);
  ESP_LOGCONFIG(TAG, "  Initial Set Seconds: %d", this->set_seconds_);
  ESP_LOGCONFIG(TAG, "  HA Sync Enabled: %s", this->enable_ha_sync_ ? "true" : "false");
  LOG_SENSOR("  ", "Remaining Seconds", this->remaining_seconds_sensor_);
  LOG_SENSOR("  ", "Set Seconds", this->set_seconds_sensor_);
  LOG_TEXT_SENSOR("  ", "State", this->state_text_sensor_);
  LOG_BINARY_SENSOR("  ", "Running", this->running_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Paused", this->paused_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Overdue", this->overdue_binary_sensor_);
}

void KitchenTimerComponent::set_max_duration_seconds(uint32_t value) {
  this->max_duration_seconds_ = value;
  this->clamp_set_and_remaining_();
}

void KitchenTimerComponent::set_seconds(int seconds) {
  int clamped = this->clamp_seconds_(seconds);
  if (this->set_seconds_ == clamped) {
    return;
  }
  this->set_seconds_ = clamped;
  this->publish_state_();
}

void KitchenTimerComponent::start() { this->start_(this->set_seconds_, false); }

void KitchenTimerComponent::start(int seconds) { this->start_(seconds, false); }

void KitchenTimerComponent::pause(bool from_ha) {
  if (this->state_ != TimerState::RUNNING) {
    return;
  }
  this->state_ = TimerState::PAUSED;
  this->publish_state_();
  for (auto *trigger : this->paused_triggers_) {
    trigger->trigger(from_ha);
  }
}

void KitchenTimerComponent::resume(bool from_ha) {
  if (this->state_ != TimerState::PAUSED) {
    return;
  }
  this->state_ = TimerState::RUNNING;
  this->synched_ = this->enable_ha_sync_ &&
                   (this->ha_state_sensor_ != nullptr || this->ha_remaining_sensor_ != nullptr);
  this->publish_state_();
  for (auto *trigger : this->resumed_triggers_) {
    trigger->trigger(from_ha);
  }
}

void KitchenTimerComponent::cancel(bool from_ha) {
  this->state_ = TimerState::STOPPED;
  this->remaining_seconds_ = 0;
  this->overdue_ = false;
  this->synched_ = false;
  this->publish_state_();
  for (auto *trigger : this->cancelled_triggers_) {
    trigger->trigger(from_ha);
  }
}

void KitchenTimerComponent::start_(int seconds, bool from_ha) {
  int clamped = this->clamp_seconds_(seconds);
  this->set_seconds_ = clamped;
  this->remaining_seconds_ = clamped;
  this->state_ = TimerState::RUNNING;
  this->overdue_ = false;
  this->synched_ = this->enable_ha_sync_ &&
                   (this->ha_state_sensor_ != nullptr || this->ha_remaining_sensor_ != nullptr);
  this->publish_state_();
  for (auto *trigger : this->started_triggers_) {
    trigger->trigger(from_ha);
  }
}

void KitchenTimerComponent::tick_() {
  if (this->state_ != TimerState::RUNNING) {
    return;
  }

  this->remaining_seconds_ -= 1;

  if (this->remaining_seconds_ <= 0 && !this->overdue_) {
    this->overdue_ = true;
    for (auto *trigger : this->finished_triggers_) {
      trigger->trigger(false);
    }
  }

  this->publish_state_();
  for (auto *trigger : this->tick_triggers_) {
    trigger->trigger(this->remaining_seconds_);
  }
}

void KitchenTimerComponent::publish_state_() {
  if (this->remaining_seconds_sensor_ != nullptr) {
    this->remaining_seconds_sensor_->publish_state(this->remaining_seconds_);
  }
  if (this->set_seconds_sensor_ != nullptr) {
    this->set_seconds_sensor_->publish_state(this->set_seconds_);
  }
  if (this->state_text_sensor_ != nullptr) {
    this->state_text_sensor_->publish_state(this->state_to_string_());
  }
  if (this->running_binary_sensor_ != nullptr) {
    this->running_binary_sensor_->publish_state(this->state_ == TimerState::RUNNING);
  }
  if (this->paused_binary_sensor_ != nullptr) {
    this->paused_binary_sensor_->publish_state(this->state_ == TimerState::PAUSED);
  }
  if (this->overdue_binary_sensor_ != nullptr) {
    this->overdue_binary_sensor_->publish_state(this->overdue_);
  }
}

const char *KitchenTimerComponent::state_to_string_() const {
  if (this->state_ == TimerState::STOPPED) {
    return "stopped";
  }
  if (this->state_ == TimerState::PAUSED) {
    return "paused";
  }
  if (this->overdue_) {
    return "overdue";
  }
  return "running";
}

int KitchenTimerComponent::clamp_seconds_(int seconds) const {
  if (seconds < 0) {
    return 0;
  }
  if (this->max_duration_seconds_ > 0 && seconds > static_cast<int>(this->max_duration_seconds_)) {
    return static_cast<int>(this->max_duration_seconds_);
  }
  return seconds;
}

void KitchenTimerComponent::clamp_set_and_remaining_() {
  this->set_seconds_ = this->clamp_seconds_(this->set_seconds_);
  this->remaining_seconds_ = this->clamp_seconds_(this->remaining_seconds_);
}

void KitchenTimerComponent::mark_synced_from_ha_() {
  if (!this->enable_ha_sync_) {
    return;
  }
  this->synched_ = (this->ha_state_sensor_ != nullptr || this->ha_remaining_sensor_ != nullptr);
}

void KitchenTimerComponent::handle_ha_state_(const std::string &state) {
  if (!this->enable_ha_sync_) {
    return;
  }

  this->mark_synced_from_ha_();

  if (state == "idle") {
    if (this->state_ != TimerState::STOPPED && this->remaining_seconds_ > 0) {
      this->cancel(true);
    }
    return;
  }

  if (state == "paused") {
    if (this->state_ == TimerState::RUNNING) {
      this->pause(true);
    } else if (this->state_ == TimerState::STOPPED) {
      // Adopt HA pause state if we were stopped
      int ha_remaining = this->ha_remaining_sensor_ != nullptr ? (int) this->ha_remaining_sensor_->state : 0;
      if (ha_remaining > 0) {
        this->set_seconds_ = this->clamp_seconds_(ha_remaining);
        this->remaining_seconds_ = ha_remaining;
        this->state_ = TimerState::PAUSED;
        this->overdue_ = false;
        this->publish_state_();
      }
    }
    return;
  }

  if (state == "active") {
    if (this->state_ == TimerState::PAUSED) {
      this->resume(true);
    } else if (this->state_ == TimerState::STOPPED) {
      int ha_remaining = this->ha_remaining_sensor_ != nullptr ? (int) this->ha_remaining_sensor_->state : 0;
      if (ha_remaining <= 0) {
        ha_remaining = this->set_seconds_ > 0 ? this->set_seconds_ : 0;
      }
      this->start_(ha_remaining, true);
    }
    return;
  }
}

void KitchenTimerComponent::handle_ha_remaining_(float value) {
  if (!this->enable_ha_sync_) {
    return;
  }

  this->mark_synced_from_ha_();

  if (value <= 0.5f) {
    return;
  }

  if (this->state_ == TimerState::STOPPED) {
    // If HA is counting while we were stopped, adopt the remaining value and mark running
    int remaining = static_cast<int>(value);
    if (remaining > 0) {
      this->set_seconds_ = this->clamp_seconds_(remaining);
      this->remaining_seconds_ = remaining;
      this->state_ = TimerState::RUNNING;
      this->overdue_ = false;
      this->publish_state_();
    }
    return;
  }

  const uint32_t now = millis();
  if (now - this->last_ha_sync_ms_ < this->sync_interval_ms_) {
    return;
  }

  int remaining = static_cast<int>(value);
  if (remaining <= 0) {
    return;
  }

  this->remaining_seconds_ = remaining;
  this->last_ha_sync_ms_ = now;
  this->publish_state_();
}

}  // namespace kitchen_timer
}  // namespace esphome
