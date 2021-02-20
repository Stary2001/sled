// Raspberry Pi Pico ws2812b output module.
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

// Matrix order and size
#if !defined(MATRIX_ORDER_PLAIN) && !defined(MATRIX_ORDER_SNAKE)
#define MATRIX_ORDER_SNAKE // cause that's what i have, it's also the easiest to wire, IMO.
#endif

#ifndef MATRIX_X
#error Define MATRIX_X as the matrixes X size.
#endif

#ifndef MATRIX_Y
#error Define MATRIX_Y as the matrixes Y size.
#endif

#if !defined(COLOR_ORDER_RGB) && !defined(COLOR_ORDER_GRB)
#define COLOR_ORDER_GRB // for now, neopixel/ws2812b style
#endif

#include <types.h>
#include <string.h>
#include <timers.h>
#include <matrix.h>

#include "hardware/pio.h"

// Calculation for amount of bytes needed.

#define ROW_SIZE MATRIX_X * 3 // 3 for R, G and B.
#define BUFFER_SIZE ROW_SIZE * MATRIX_Y
#define MATRIX_PIXELS (MATRIX_X * MATRIX_Y)

uint32_t leds[MATRIX_PIXELS];

#include <stdint.h>
#include <stdio.h>

/// XXXXXXXXXXXX todo
// awful shitty blocking implementation: now
// core1 for sending neopixel data: later

void setup_ws2812(int pin);
int init(void) {
	setup_ws2812(2);
	memset(leds, 0, sizeof(leds));
	return 0;
}

int getx(int _modno) {
	return MATRIX_X; // for now.
}
int gety(int _modno) {
	return MATRIX_Y; // for now.
}

int ppos(int x, int y) {
#if defined(MATRIX_ORDER_PLAIN)
	return (x + (y * MATRIX_X));
#elif defined(MATRIX_ORDER_SNAKE)
	// Order is like this
	// 0 1 2
	// 5 4 3
	// 6 7 8
	return (((y % 2) == 0 ? x : (MATRIX_X - 1) - x) + MATRIX_X*y);
#endif
}

int set(int _modno, int x, int y, RGB color) {
	// No OOB check, because performance.
#ifdef COLOR_ORDER_RGB
	uint32_t led = ((color.red) << 16) | (color.green << 8) | color.blue;
#elif defined(COLOR_ORDER_GBR)
	uint32_t led = (color.green << 16) | (color.blue << 8) | color.red;
#elif defined(COLOR_ORDER_GRB)
	uint32_t led = (color.green << 16) | (color.red << 8) | color.blue;
#else
#error Must define color order.
#endif
	leds[ppos(x, y)] = led;
	return 0;
}


RGB get(int _modno, int x, int y) {
	// No OOB check, because performance.
	uint32_t led = leds[ppos(x, y)];
#ifdef COLOR_ORDER_RGB
	return RGB((led >> 16) & 0xFF, (led >> 8) & 0xFF, led & 0xFF);
#elif defined(COLOR_ORDER_GBR)
	return RGB(led & 0xFF, (led >> 16) & 0xFF, (led >> 8) & 0xFF);
#elif defined(COLOR_ORDER_GRB)
	return RGB((led >> 8) & 0xFF, (led >> 16) & 0xFF, led & 0xFF);
#else
#error Must define color order.
#endif
}


// Zeroes the stuff.
RGB black = RGB(0, 0, 0);
int clear(int _modno) {
	matrix_fill(0, 0, MATRIX_X - 1, MATRIX_Y - 1, black);
	return 0;
}

static void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

int render(void) {
	for(int i = 0; i < MATRIX_PIXELS; i++) {
		put_pixel(leds[i]);
	}
	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	return timers_wait_until_core(desired_usec);
}

void wait_until_break(int _modno) {
	return timers_wait_until_break_core();
}

void deinit(int _modno) {

}
