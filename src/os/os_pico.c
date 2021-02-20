// os_pico: for non-multithreaded systems only
// Lots of stubs.
//
// Copyright (c) 2021, Ezekiel Bethel <zek@9net.org>
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

#include "../types.h"
#include "../oscore.h"
#include "../main.h"
#include "../timers.h"
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"

// -- event
oscore_event oscore_event_new(void) {
	return NULL;
}

int oscore_event_wait_until(oscore_event ev, oscore_time desired_usec) {
	// XXX ideally actual event wait

	oscore_time tnow = time_us_64();
	if (tnow >= desired_usec)
		return tnow;
	oscore_time sleeptime = desired_usec - tnow;
	sleep_us(sleeptime);
	return 0;
}

void oscore_event_signal(oscore_event ev) {
}

void oscore_event_free(oscore_event ev) {
}

// Time keeping.
// Note, this should be replaced.
oscore_time oscore_udate(void) {
	return time_us_64();
}

// Threading
oscore_task oscore_task_create(const char* name, oscore_task_function func, void* ctx) {
	// uuh
	return NULL;
}

void oscore_task_setprio(oscore_task task, int prio) {
	// nothing
}

void oscore_task_yield(void) {
	// nothing.
};

void oscore_task_exit(void * status) {
	// nope
};

void * oscore_task_join(oscore_task task) {
	// ye ok
	return 0;
};

int oscore_ncpus(void) {
	return 1;
}

void oscore_task_pin(oscore_task task, int cpu) {}

// -- mutex
oscore_mutex oscore_mutex_new(void) {
	return NULL;
}

void oscore_mutex_lock(oscore_mutex m) {
}

void oscore_mutex_unlock(oscore_mutex m) {
}

void oscore_mutex_free(oscore_mutex m) {
}

void setup_ws2812(int pin) {
	PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, pin, 800000, false);
}