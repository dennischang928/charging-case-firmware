#include <zephyr/kernel.h>
#include "led_fx.h"


int main(void)
{
	if (led_fx_init()) {
		printk("strip not ready\n");
		return 0;
	}

	while (1) {
		
		bounce();                        /* accelerate / decelerate (default) */
		// bounce(.ease = ease_spring); 
	}
}

