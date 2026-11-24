# AGENTS.md: `chg_case` board definition

Zephyr board for the CHG_CASE PCB (schematic `CHG_CASE.pdf`, Nov 11, kept with the Cicada hardware files, not in this repo). It was derived from Zephyr's `weact_stm32g030_core` (same STM32G030F6P6, TSSOP-20), with every peripheral re-mapped to CHG_CASE nets. The schematic wins over this file. Flag any conflict rather than editing the hardware docs.

## Files

| File | Role |
|---|---|
| `board.yml` | Board name `chg_case`, SoC `stm32g030xx` |
| `Kconfig.chg_case` | `BOARD_CHG_CASE` selects `SOC_STM32G030XX` |
| `chg_case.dts` | Pins, clocks, console, SPI (LEDs), ADC (CHG_IMON), named GPIOs in `zephyr,user` |
| `chg_case_defconfig` | Serial + console + GPIO on by default |
| `board.cmake`, `support/openocd.cfg` | Flash runners. The first `include()` is the default (STM32CubeProgrammer), so pass `--runner openocd` |

Changing anything here needs a pristine build (`west build -p …`).

## Pin map (schematic → devicetree)

| Pkg pin | Port | Net | Devicetree | Notes |
|---|---|---|---|---|
| 1 | PB7 (PB8 bonded) | CHG_RX | not yet | USART1 RX from retainer, J5-1 |
| 2/3 | PC14/PC15 | OSC32 | `&clk_lse` | Y1 32.768 kHz |
| 7 | PA0 | LID_HALL_N | `lid-hall-gpios` | from 5 V Hall via R11/R12 divider (~3.3–3.7 V high). Check PA0 tolerance |
| 8 | PA1 | DEV_SENSE | `dev-sense-gpios` | J2 Hall, R4 10k pull-up. Keep free of SPI SCK |
| 9/10 | PA2/PA3 | DBG_TX/RX | `&usart2`, console | J8 |
| 11 | PA4 | USB_FLT_N | `usb-flt-gpios` | TPS25200 fault, R1 10k pull-up |
| 12/13 | PA5/PA6 | CC1/CC2_SENSE | not yet | USB-C CC lines. Never put a clock here |
| 14 | PA7 | LED_DIN | `&spi1` MOSI | → U2 SN74LV1T34 (5 V) → R2 330 Ω → J1-2 |
| 15 | PB0 (PB1/PB2/PA8 bonded) | EFUSE_CHG_EN | `chg-en-gpios` | R16 100k pull-down keeps charge off in reset |
| 16 | PA11 | UV_PWM | not yet | TIM1_CH4. Must never be pulled or driven high by accident |
| 17 | PA12 | CHG_IMON | `&adc1` ch 16, `io-channels` | R18 3.83k to GND, ~2.51 mV/mA |
| 18/19 | PA13/PA14 | SWDIO/SWCLK | (SWD) | J6 |
| 20 | PB3 (PB4–PB6 bonded) | CHG_FLT_N | `chg-flt-gpios` + `GPIO_PULL_UP` | open-drain, **no external pull-up**, so the internal one is required |

On bonded pins, configure only the listed port, and never drive a second port of the same pin.

## Devicetree gotchas

- **SPI1 must stay half-duplex** (`duplex = <SPI_HALF_DUPLEX>` on `ws2812b@0`, needs `#include <zephyr/dt-bindings/spi/spi.h>`). Full-duplex without an SCK pin hangs in `spi_ll_stm32.c` waiting for RXNE. The only SCK-capable pins (PA1, PA5, PB3) are all CHG_CASE nets.
- **Pinctrl file name** is `stm32g030f6px-pinctrl.dtsi` (no parentheses). Valid names are `peripheral_signal_pin`, e.g. `grep "_pa7:" ~/zephyrproject/modules/hal/stm32/dts/st/g0/stm32g030f6px-pinctrl.dtsi`.
- **ADC node format is version-specific.** In Zephyr 4.3 the clocks/prescaler live on `&adc1`. Newer Zephyr moves them to `&adc1_common`. Copy from a board in the same Zephyr version.
- **GPIO flags are all `GPIO_ACTIVE_HIGH`**, even on `_N` nets, so code reads raw pin levels (0 = asserted for `_N` signals).
- Known comment errors in `chg_case.dts`: USB_FLT_N is package pin 11 (not 9), and the copyright header is still WeAct's.
- `board.cmake` passes `--device=STM32G031K8` to J-Link. The correct value is `STM32G030F6`. It only matters for J-Link.
- `zephyr.dts` in the build folder is generated. Edit `chg_case.dts`, never the output.
