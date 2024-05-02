#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include "mbe_can.h"
#include "ws2812.h"

#define NEOPIXEL_PIN 16
#define NEOPIXEL_NUM_PIXELS 8
#define NEOPIXEL_INTERVAL_MS 10

#define SLOW_FLASH(ts) (((ts / 300000) % 2) == 0)
#define FAST_FLASH(ts) (((ts / 100000) % 2) == 0)

#define BUTTON_PIN 22
#define BUTTON_REPEAT_MS 200
#define BRIGHTNESS_DISPLAY_MS 500
#define BRIGHTNESS_MAX 8

// Note: Sigma 125 has soft cut at 6800 RPM / hard cut at 6900 RPM.
const uint16_t RPM_THRESHOLDS[] = {
  4100,
  4400,
  4700,
  5000,
  5300,
  5600,
  5900,
  6200,
  6500,
};

const uint32_t RPM_COLOURS[] = {
  WS2812_GREEN,
  WS2812_GREEN,
  WS2812_GREEN,
  WS2812_ORANGE,
  WS2812_ORANGE,
  WS2812_ORANGE,
  WS2812_RED,
  WS2812_RED,
};

const uint8_t RPM_COUNT = 8;

static absolute_time_t next_update;

static absolute_time_t show_brightness_until;

static absolute_time_t button_locked_until;

static uint8_t brightness = 0;

void update_brightness(absolute_time_t ts);
void update_leds(absolute_time_t ts);

int main() {
  stdio_init_all();

  mbe_can_setup();

  ws2812_setup(pio1, NEOPIXEL_PIN, NEOPIXEL_NUM_PIXELS);

  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN);

  next_update = show_brightness_until = button_locked_until = from_us_since_boot(0);

  while (1) {
    mbe_can_update();

    absolute_time_t ts = get_absolute_time();
    update_brightness(ts);
    if (ts >= next_update) {
      next_update = delayed_by_ms(ts, NEOPIXEL_INTERVAL_MS);
      update_leds(ts);
    }
  }
}

void update_brightness(absolute_time_t ts) {
  if (ts < button_locked_until) {
    return;
  }
  if (!gpio_get(BUTTON_PIN)) {
    button_locked_until = delayed_by_ms(ts, BUTTON_REPEAT_MS);
    show_brightness_until = delayed_by_ms(ts, BRIGHTNESS_DISPLAY_MS);
    brightness = (brightness + 1) % BRIGHTNESS_MAX;
  }
}

void update_leds(absolute_time_t ts) {
  ws2812_clear();

  bool unresponsive = !mbe_can_is_data_valid();
  uint16_t rpm = mbe_can_rpm();

  if (ts < show_brightness_until) {
    for (uint8_t i = 0; i <= brightness; i++) {
      ws2812_set_pixel(i, WS2812_GREEN * (brightness + 1));
    }
  } else if (unresponsive) {
    // No response from ECU.
    // Slow red flash first LED.
    if (SLOW_FLASH(ts)) {
      ws2812_set_pixel(0, WS2812_RED * (brightness + 1));
    }
  } else if (rpm == 0) {
    // ECU responding but engine not running.
    // Slow orange flash first LED.
    if (SLOW_FLASH(ts)) {
      ws2812_set_pixel(0, WS2812_ORANGE * (brightness + 1));
    }
  } else if (rpm >= RPM_THRESHOLDS[RPM_COUNT]) {
    // RPM above upper threshold.
    // Fast red flash, all LEDs.
    if (FAST_FLASH(ts)) {
      for (uint8_t i = 0; i < RPM_COUNT; i++) {
        ws2812_set_pixel(i, WS2812_RED * (brightness + 1));
      }
    }
  } else {
    // RPM nonzero, but below upper threshold.
    // Set LEDs according to defined thresholds / colors.
    for (uint8_t i = 0; i < RPM_COUNT; i++) {
      if (rpm >= RPM_THRESHOLDS[i]) {
        ws2812_set_pixel(i, RPM_COLOURS[i] * (brightness + 1));
      }
    }
  }

  ws2812_show();
}
