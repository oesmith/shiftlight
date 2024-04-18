/**
 * Derived from the Raspberry Pi Pico SDK example at
 * https://github.com/raspberrypi/pico-examples/tree/eca13acf57916a0bd5961028314006983894fc84/pio/ws2812
 *
 * Original copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.h"
#include "ws2812.pio.h"

static PIO ws2812_pio;
static uint ws2812_sm;
static uint ws2812_num_pixels;
static uint32_t *ws2812_data;

void ws2812_setup(PIO pio, uint pin, uint num_pixels) {
  ws2812_pio = pio;
  ws2812_num_pixels = num_pixels;

  ws2812_data = (uint32_t*) malloc(4 * num_pixels);
  ws2812_clear();

  ws2812_sm = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, ws2812_sm, offset, pin);
}

void ws2812_clear() {
  memset(ws2812_data, 0, 4 * ws2812_num_pixels);
}

void ws2812_set_pixel(uint n, uint32_t color) {
  if (n >= ws2812_num_pixels) {
    return;
  }
  ws2812_data[n] = color;
}

void ws2812_show() {
  // Note: the TX FIFO depth is 8x 32-bit words (the SM is configured with
  // PIO_FIFO_JOIN_TX), so this won't actually block as long as the FIFO is
  // exhausted before calling this method _and_ the number of pixels is 8 or
  // fewer.
  for (int i = 0; i < ws2812_num_pixels; i++) {
    pio_sm_put_blocking(ws2812_pio, ws2812_sm, ws2812_data[i]);
  }
}
