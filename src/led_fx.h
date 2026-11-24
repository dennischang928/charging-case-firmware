#pragma once

#include <zephyr/drivers/led_strip.h>

#define STRIP_NODE DT_NODELABEL(led_strip)
#define NUM_LEDS   DT_PROP(STRIP_NODE, chain_length)

float ease_linear(float u);
float ease_in_out(float u);
float ease_spring(float u);

struct bounce_cfg {
	struct led_rgb color;      /* default: white (200) */
	float (*ease)(float u);    /* default: ease_in_out */
	uint16_t ms;               /* one-way trip time, default: 1000 */
};


int led_fx_init(void); // called once, 0 -> ok

void bounce_run(struct bounce_cfg c);

void update_color(struct led_rgb colors[static NUM_LEDS]);



/* bounce();  bounce(.ease = ease_spring, .color = { .b = 255 }, .ms = 600); */
#define bounce(...) bounce_run((struct bounce_cfg){ __VA_ARGS__ })