/*
 * led_fx: status-LED effects for the CHG_CASE WS2812B chain (UV_LID D1–D4).
 * API in led_fx.h. Strip comes from devicetree node label `led_strip`
 * (worldsemi,ws2812-spi on SPI1, see boards/cicada/chg_case/chg_case.dts).
 *
 * Note: STM32G0 SPI1 hangs waiting for RXNE unless an SCK pin is assigned;
 * SCK is currently on PA1 (steals DEV_SENSE, unplug J2).
 */
#include <math.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "led_fx.h"

/* Tuning knobs */
#define STRIP_NODE DT_NODELABEL(led_strip)
#define NUM_LEDS   DT_PROP(STRIP_NODE, chain_length)
#define SUB        50     /* position resolution: sub-steps per LED */
#define WIDTH      120    /* glow radius in sub-steps (50 = 1 LED) */
#define FRAME_MS   8      /* frame period; >280 µs needed for WS2812B latch */
#define DAMPING    5.0f   /* spring: 2 = loose, 10 = firm */
#define WOBBLES    3.0f   /* spring: wobbles per trip */

/* Motion curves: u = trip progress 0..1 → glow position 0..1 (spring overshoots). */
float ease_linear(float u) { return u; }
float ease_in_out(float u) { return u * u * (3 - 2 * u); }   /* smoothstep */
/* ponytail: soft-float expf/cosf add ~5 KB flash on M0+ (only linked if ease_spring is used);
 * swap for a lookup table if space runs out */
float ease_spring(float u) { return 1 - expf(-DAMPING * u) * cosf(WOBBLES * 2 * 3.14159f * u); }

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[NUM_LEDS];

int led_fx_init(void)
{
	return device_is_ready(strip) ? 0 : -ENODEV;
}

/* Blocking: one round trip (out + back) takes 2 * c.ms.
 * ponytail: blocks the caller; run from its own thread once case logic (UV, lid) lands. */
void bounce_run(struct bounce_cfg c)
{
	/* zero/omitted field → default */
	if (!c.color.r && !c.color.g && !c.color.b) {
		c.color = (struct led_rgb){ .r = 200, .g = 200, .b = 200 };
	}
	if (!c.ease) c.ease = ease_in_out;
	if (!c.ms)   c.ms = 1000;

	const int span = (NUM_LEDS - 1) * SUB;   /* first LED → last, in sub-steps */
	const int steps = c.ms / FRAME_MS;

	for (int trip = 0; trip < 2; trip++) {                       /* 0 = out, 1 = back */
		for (int s = 0; s <= steps; s++) {
			float e = c.ease((float)s / steps);
			int pos = trip ? (1 - e) * span : e * span;          /* glow center */

			/* refill every frame: led_strip_update_rgb() may overwrite pixels */
			for (int i = 0; i < NUM_LEDS; i++) {
				int d = abs(i * SUB - pos);                       /* distance to glow center */
				int b = d >= WIDTH ? 0 : 255 * (WIDTH - d) * (WIDTH - d) / (WIDTH * WIDTH);   /* squared falloff */

				pixels[i] = (struct led_rgb){
					.r = c.color.r * b / 255,
					.g = c.color.g * b / 255,
					.b = c.color.b * b / 255,
				};
			}
			led_strip_update_rgb(strip, pixels, NUM_LEDS);
			k_msleep(FRAME_MS);
		}
	}
}
