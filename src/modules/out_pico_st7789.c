// Raspberry Pi Pico ST7789 LCD output module.
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
#define COLOR_ORDER_RGB
#endif

#include <types.h>
#include <string.h>
#include <timers.h>
#include <matrix.h>
#include <colors.h>

#include "hardware/pio.h"

// Calculation for amount of bytes needed.

#define ROW_SIZE MATRIX_X * 3 // 3 for R, G and B.
#define BUFFER_SIZE ROW_SIZE * MATRIX_Y
#define MATRIX_PIXELS (MATRIX_X * MATRIX_Y)

uint32_t leds[MATRIX_PIXELS];

#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "st7789_lcd.pio.h"

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

#define PIN_DIN 0
#define PIN_CLK 1
#define PIN_CS 2
#define PIN_DC 3
#define PIN_RESET 4
#define PIN_BL 5

#define SERIAL_CLK_DIV 1.f

/// XXXXXXXXXXXX todo
// awful shitty blocking implementation: now
// core1 for sending lcd data: later


/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Format: cmd length (including cmd byte), post delay in units of 5 ms, then cmd payload
// Note the delays have been shortened a little
static const uint8_t st7789_init_seq[] = {
        1, 20, 0x01,                         // Software reset
        1, 10, 0x11,                         // Exit sleep mode
        2, 2, 0x3a, 0x55,                   // Set colour mode to 16 bit
        2, 0, 0x36, 0x00,                   // Set MADCTL: row then column, refresh is bottom to top ????
        5, 0, 0x2a, 0x00, 0x00, 0x00, 0xf0, // CASET: column addresses from 0 to 240 (f0)
        5, 0, 0x2b, 0x00, 0x00, 0x00, 0xf0, // RASET: row addresses from 0 to 240 (f0)
        1, 2, 0x21,                         // Inversion on, then 10 ms delay (supposedly a hack?)
        1, 2, 0x13,                         // Normal display on, then 10 ms delay
        1, 2, 0x29,                         // Main screen turn on, then wait 500 ms
        0                                     // Terminate list
};

static inline void lcd_set_dc_cs(bool dc, bool cs) {
    sleep_us(1);
    gpio_put_masked((1u << PIN_DC) | (1u << PIN_CS), !!dc << PIN_DC | !!cs << PIN_CS);
    sleep_us(1);
}

static inline void lcd_write_cmd(PIO pio, uint sm, const uint8_t *cmd, size_t count) {
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(0, 0);
    st7789_lcd_put(pio, sm, *cmd++);
    if (count >= 2) {
        st7789_lcd_wait_idle(pio, sm);
        lcd_set_dc_cs(1, 0);
        for (size_t i = 0; i < count - 1; ++i)
            st7789_lcd_put(pio, sm, *cmd++);
    }
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(1, 1);
}

static inline void lcd_init(PIO pio, uint sm, const uint8_t *init_seq) {
    const uint8_t *cmd = init_seq;
    while (*cmd) {
        lcd_write_cmd(pio, sm, cmd + 2, *cmd);
        sleep_ms(*(cmd + 1) * 5);
        cmd += *cmd + 2;
    }
}

static inline void st7789_start_pixels(PIO pio, uint sm) {
    uint8_t cmd = 0x2c; // RAMWR
    lcd_write_cmd(pio, sm, &cmd, 1);
    lcd_set_dc_cs(1, 0);
}

PIO pio = pio0;
uint sm = 0;

int init(void) {
    uint offset = pio_add_program(pio, &st7789_lcd_program);
    st7789_lcd_program_init(pio, sm, offset, PIN_DIN, PIN_CLK, SERIAL_CLK_DIV);

    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RESET);
    gpio_init(PIN_BL);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RESET, GPIO_OUT);
    gpio_set_dir(PIN_BL, GPIO_OUT);

    gpio_put(PIN_CS, 1);
    gpio_put(PIN_RESET, 1);
    lcd_init(pio, sm, st7789_init_seq);
    gpio_put(PIN_BL, 1);

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

int render(void) {
    st7789_start_pixels(pio, sm);
    for(int y = 0; y < SCREEN_HEIGHT; y++) {
        for(int x = 0; x < SCREEN_WIDTH/4; x++) {
            uint16_t colour = RGB2RGB565(get(0,x,y/4));
            
            for(int i = 0; i<4; i++) {
                st7789_lcd_put(pio, sm, colour >> 8);
                st7789_lcd_put(pio, sm, colour & 0xff);
            }
        }
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