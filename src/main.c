#include <zephyr/kernel.h>
#include "led_fx.h"



struct led_rgb pixels[NUM_LEDS];



int main(void)
{
	if (led_fx_init()) {
		printk("strip not ready\n");
		return 0;
	}
	pixels[0].r = 255;
	pixels[0].g = 0;
	pixels[0].b = 0;
	update_color(pixels);

	// while (1) {
	// 	bounce(.ease = ease_spring);
	// 	bounce();                        /* accelerate / decelerate (default) */
	// 	// bounce(.ease = ease_linear);     /* constant speed */
	// }
}