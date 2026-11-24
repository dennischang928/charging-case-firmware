#pragma once

#include <zephyr/drivers/led_strip.h>

float ease_linear(float u);
float ease_in_out(float u);
float ease_spring(float u);

struct bounce_cfg {
	struct led_rgb color;      /* default: white (200) */
	float (*ease)(float u);    /* default: ease_in_out */
	uint16_t ms;               /* one-way trip time, default: 1000 */
};

void bounce_run(struct bounce_cfg c);

int led_fx_init(void); // called once, 0 -> ok

/* bounce();  bounce(.ease = ease_spring, .color = { .b = 255 }, .ms = 600); */
#define bounce(...) bounce_run((struct bounce_cfg){ __VA_ARGS__ })