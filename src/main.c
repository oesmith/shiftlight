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

#define RPM_GREEN (WS2812_GREEN * 3)
#define RPM_ORANGE (WS2812_ORANGE * 3)
#define RPM_RED (WS2812_RED * 3)

const uint16_t RPM_THRESHOLDS[] = {
  4000,
  4300,
  4600,
  4900,
  5200,
  5500,
  5800,
  6100,
  6400,
};

const uint32_t RPM_COLOURS[] = {
  RPM_GREEN,
  RPM_GREEN,
  RPM_GREEN,
  RPM_ORANGE,
  RPM_ORANGE,
  RPM_ORANGE,
  RPM_RED,
  RPM_RED,
};

const uint8_t RPM_COUNT = 8;

static absolute_time_t next_update;

void update_leds(absolute_time_t ts);

int main() {
  stdio_init_all();

  mbe_can_setup();
  ws2812_setup(pio1, NEOPIXEL_PIN, NEOPIXEL_NUM_PIXELS);

  next_update = from_us_since_boot(0);

  while (1) {
    mbe_can_update();

    absolute_time_t ts = get_absolute_time();
    if (ts >= next_update) {
      next_update = delayed_by_ms(ts, NEOPIXEL_INTERVAL_MS);
      update_leds(ts);
    }
  }
}

void update_leds(absolute_time_t ts) {
  ws2812_clear();

  bool unresponsive = !mbe_can_is_data_valid();
  uint16_t rpm = mbe_can_rpm();

  if (unresponsive) {
    // No response from ECU.
    // Slow red flash first LED.
    if (SLOW_FLASH(ts)) {
      ws2812_set_pixel(0, WS2812_RED);
    }
  } else if (rpm == 0) {
    // ECU responding but engine not running.
    // Slow orange flash first LED.
    if (SLOW_FLASH(ts)) {
      ws2812_set_pixel(0, WS2812_ORANGE);
    }
  } else if (rpm >= RPM_THRESHOLDS[RPM_COUNT]) {
    // RPM above upper threshold.
    // Fast red flash, all LEDs.
    if (FAST_FLASH(ts)) {
      for (uint8_t i = 0; i < RPM_COUNT; i++) {
        ws2812_set_pixel(i, RPM_RED);
      }
    }
  } else {
    // RPM nonzero, but below upper threshold.
    // Set LEDs according to defined thresholds / colors.
    for (uint8_t i = 0; i < RPM_COUNT; i++) {
      if (rpm >= RPM_THRESHOLDS[i]) {
        ws2812_set_pixel(RPM_COUNT - 1 - i, RPM_COLOURS[i]);
      }
    }
  }

  ws2812_show();
}
