#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include "led_fx.h"

/* CHG_IMON (PA12): first entry of io-channels in zephyr,user */
static const struct adc_dt_spec imon = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

#define IMON_SAMPLES 16   /* average to smooth the ±1 mV (≈0.5 mA) flicker */

static int32_t imon_mv(void)
{
	int16_t raw;
	struct adc_sequence seq = { .buffer = &raw, .buffer_size = sizeof(raw) };
	int32_t sum = 0;

	adc_sequence_init_dt(&imon, &seq);
	for (int i = 0; i < IMON_SAMPLES; i++) {
		if (adc_read_dt(&imon, &seq)) {
			return -1;
		}
		sum += raw;
	}
	int32_t mv = sum / IMON_SAMPLES;

	adc_raw_to_millivolts_dt(&imon, &mv);
	return mv;
}

/* Bench two-point calibration (meter in series with a J5 load), CHG_CASE board 2026-11-23:
 *   7.43 mA → 30 mV,  15.32 mA → 45 mV   (≈0.53 mA/mV, zero ≈ 16 mV)
 * ponytail: linear fit from two low-current points; TPS259631 IMON is only specified
 * 0.125–2 A, so re-calibrate with a higher load before trusting readings above ~20 mA. */
#define CAL_MV1  30
#define CAL_UA1  7430
#define CAL_MV2  45
#define CAL_UA2  15320

static int32_t imon_ua(int32_t mv)
{
	int32_t ua = CAL_UA1 + (mv - CAL_MV1) * (CAL_UA2 - CAL_UA1) / (CAL_MV2 - CAL_MV1);

	return ua < 5000 ? 0 : ua;           /* < 5 mA is IMON offset/noise (out of spec) → 0 */
}

static const struct gpio_dt_spec chg_en = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), chg_en_gpios);

int chg_init(void){
	if(!gpio_is_ready_dt(&chg_en)) {
		return -ENODEV;
	}
	return gpio_pin_configure_dt(&chg_en, GPIO_OUTPUT_INACTIVE);
}

void chg_set(bool on)
{
	gpio_pin_set_dt(&chg_en, on);
}

int main(void)
{
	if(chg_init()) {
		printk("charging enable not ready\n");
		return 0;
	}
	if (!adc_is_ready_dt(&imon) || adc_channel_setup_dt(&imon)) {
		printk("adc not ready\n");
		return 0;
	}
	// if (led_fx_init()) {
	// 	printk("strip not ready\n");
	// 	return 0;
	// }

	while (1) {
		chg_set(true);

		/* LEDs off while sampling: their ~100 mA ground return shifts CHG_IMON by ~25 mV */
		static struct led_rgb off[NUM_LEDS];
		update_color(off);
		k_msleep(5);

		int32_t mv = imon_mv();          /* one (averaged) read, so mV and mA match */
		int32_t ua = imon_ua(mv);
		printk("chg ON  imon=%d mV  %d.%02d mA\n", mv, ua / 1000, (ua % 1000) / 10);
		// k_msleep(500);
		
		// printk("HelloWorld\n");
		bounce();                        /* accelerate / decelerate (default) */
		// bounce(.ease = ease_spring);
	}
}

