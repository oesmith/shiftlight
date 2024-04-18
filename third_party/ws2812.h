#ifndef _WS2812_H
#define _WS2812_H

#include "hardware/pio.h"

#define WS2812_PIN 2
#define WS2812_NUM_PIXELS 8

// Colours are specified in GRBW order, but we're only using the top 24 bits.
#define WS2812_RED    0x00110000
#define WS2812_GREEN  0x11000000
#define WS2812_BLUE   0x00001100
#define WS2812_ORANGE 0x11110000

void ws2812_setup(PIO pio, uint pin, uint num_pixels);
void ws2812_clear();
void ws2812_set_pixel(uint n, uint32_t color);
void ws2812_show();

#endif // _WS2812_H
