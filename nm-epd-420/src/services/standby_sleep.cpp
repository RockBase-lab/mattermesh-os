#include "standby_sleep.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_timer.h>

#include "board/hardware.h"

namespace device {
namespace {
StandbySleepState state;

void disableWakePin(int pin) {
  gpio_wakeup_disable(static_cast<gpio_num_t>(pin));
}
} // namespace

bool standbyLightSleep(uint32_t maxMs) {
  if (!maxMs || digitalRead(BOARD_BOOT_PIN) == LOW || digitalRead(BOARD_USER_BUTTON) == LOW ||
      digitalRead(BOARD_LORA_INT) == HIGH)
    return false;

  const gpio_num_t boot = static_cast<gpio_num_t>(BOARD_BOOT_PIN);
  const gpio_num_t user = static_cast<gpio_num_t>(BOARD_USER_BUTTON);
  const gpio_num_t radio = static_cast<gpio_num_t>(BOARD_LORA_INT);
  if (gpio_wakeup_enable(boot, GPIO_INTR_LOW_LEVEL) != ESP_OK ||
      gpio_wakeup_enable(user, GPIO_INTR_LOW_LEVEL) != ESP_OK ||
      gpio_wakeup_enable(radio, GPIO_INTR_HIGH_LEVEL) != ESP_OK ||
      esp_sleep_enable_gpio_wakeup() != ESP_OK ||
      esp_sleep_enable_timer_wakeup(uint64_t(maxMs) * 1000ULL) != ESP_OK) {
    disableWakePin(BOARD_BOOT_PIN);
    disableWakePin(BOARD_USER_BUTTON);
    disableWakePin(BOARD_LORA_INT);
    state.errors++;
    return false;
  }

  const int64_t started = esp_timer_get_time();
  const esp_err_t result = esp_light_sleep_start();
  const int64_t elapsedUs = esp_timer_get_time() - started;
  state.lastWake = esp_sleep_get_wakeup_cause();
  disableWakePin(BOARD_BOOT_PIN);
  disableWakePin(BOARD_USER_BUTTON);
  disableWakePin(BOARD_LORA_INT);

  if (result != ESP_OK) {
    state.errors++;
    return false;
  }
  state.entries++;
  state.lastMs = elapsedUs > 0 ? uint32_t(elapsedUs / 1000) : 0;
  state.totalMs += state.lastMs;
  return true;
}

StandbySleepState standbySleepState() {
  return state;
}

} // namespace device
