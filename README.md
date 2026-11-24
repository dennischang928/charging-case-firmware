# Charging-case firmware (CHG_CASE, STM32G030)

Zephyr application for the Cicada charging-case main board (CHG_CASE, MCU **U1 STM32G030F6P6**, 32 KB flash / 8 KB RAM). This is bench bring-up firmware for a research prototype. It is not production code and not a validated medical device. Say "UV cleaning", not "sterilization".

For AI agents and a file map, start at [AGENTS.md](AGENTS.md). Board-specific notes are in [boards/cicada/chg_case/AGENTS.md](boards/cicada/chg_case/AGENTS.md).

## Status (bench, 2026-11-23)

| Feature | Pins | State |
|---|---|---|
| Boot + console | USART2 PA2/PA3 → J8, 115200 | ✅ working |
| WS2812B status LEDs (`led_fx`: `bounce()`, `update_color()`) | SPI1 MOSI PA7 → U2 → J1 | ✅ working on a commercial WS2812 board. The hand-soldered UV_LID LEDs did not light (board-side issue, open) |
| Charge output enable | PB0 `EFUSE_CHG_EN` → U6 | ✅ toggles 5 V on J5 pin 3 |
| Charge current readout | PA12 `CHG_IMON` → ADC1 IN16 | ✅ reads, two-point calibrated (see Known issues) |
| Lid Hall, device-present, eFuse/USB faults | PA0, PA1, PB3, PA4 | defined in devicetree, not used in code yet |
| UV-C PWM, retainer UART (CHG_RX), USB-C CC sense | PA11, PB7, PA5/PA6 | not started |

`src/chg_ctrl.c/.h` are empty stubs. The charge code currently lives in `main.c`.

## The custom board is required

Zephyr has no board definition for CHG_CASE, so this repo carries its own in [`boards/cicada/chg_case/`](boards/cicada/chg_case). **You must build with `-b chg_case` and point `BOARD_ROOT` at this folder** (see Build). It was started from Zephyr's `weact_stm32g030_core` (same chip), but its devicetree was rewritten for this PCB.

Do **not** build for `weact_stm32g030_core` and flash it onto CHG_CASE. That board enables I2C2 with pull-ups on PA11 (= `UV_PWM`, turns on the UV drivers when the lid is closed), blinks PA4 (= `USB_FLT_N`), and drives PA1/PA6/PB6, which are wired to other nets here.

What `chg_case.dts` customizes, and why:

| Block | What | Why |
|---|---|---|
| `chosen` | console and shell on `&usart2` | J8 is wired to PA2/PA3, so USART2 (the WeAct board uses USART1) |
| `zephyr,user` | named GPIOs (`lid-hall`, `dev-sense`, `usb-flt`, `chg-en`, `chg-flt`) + `io-channels = <&adc1 16>` | app code finds pins by name, no hard-coded pin numbers |
| `&spi1` | MOSI only (`spi1_mosi_pa7`), `duplex = <SPI_HALF_DUPLEX>` | WS2812 data. Full-duplex hangs forever without an SCK pin. Half-duplex TX needs no SCK, which keeps PA1 free for DEV_SENSE |
| `&adc1` | PA12, channel 16, `vref-mv = 3300` | CHG_IMON. Clock settings copied from Zephyr 4.3's `nucleo_g071rb` |
| clocks | HSI 16 MHz → PLL → 64 MHz, LSE on | no main crystal; Y1 is 32.768 kHz (PC14/PC15) |
| removed | WeAct `leds`, `&i2c2`, `&usart1`, full `&spi1` | their pins are CHG_CASE nets |

## Requirements

- A Zephyr **v4.3.2** west workspace, e.g. `~/zephyrproject`, with `west` in its `.venv`
- Zephyr SDK **0.17.4** (toolchain)
- OpenOCD: `brew install openocd`. STM32CubeProgrammer is not needed; pass `--runner openocd`
- An ST-Link on **J6** (SWD), and a serial port on **J8**: a USB-UART adapter, or the ST-Link V2-1's own serial port
- Serial monitor: `brew install tio` (optional)

Keep the workspace and the build folder **outside Google Drive**. This app folder works from Drive, but Drive can corrupt a `.git` folder, so move the repo out before sharing it.

## Build

Set the app path (wherever this folder is), then enter the workspace:

```bash
export APP=/path/to/charging-case-firmware
```

```bash
source ~/zephyrproject/.venv/bin/activate && cd ~/zephyrproject
```

**First build, or after changing boards, adding or renaming files, or Kconfig changes** (pristine):

```bash
west build -p -b chg_case -d ~/build $APP -- -DBOARD_ROOT=$APP
```

**Everyday rebuild** (board, app path and BOARD_ROOT are remembered in `~/build`):

```bash
west build -d ~/build
```

`west` only works inside a workspace, so always run it from `~/zephyrproject`.

## Flash

`west flash` rebuilds first, so after editing code this one command is enough:

```bash
west flash -d ~/build --runner openocd
```

Read back and compare with the build (no flashing):

```bash
west flash -d ~/build --runner openocd --verify-only
```

Success is `wrote N bytes …` with no `Error:` lines. `Target voltage … may be too low` and `Unable to match requested speed` are harmless with this ST-Link.

## Serial console (J8, 115200)

Find the port (plug the adapter in and run it again to see which entry is new):

```bash
tio --list
```

```bash
tio -b 115200 /dev/cu.usbserial-XXXX
```

Quit with `Ctrl-T q`. Without tio, this works from any shell, including ones with no PTY:

```bash
(stty 115200 raw; cat) < /dev/cu.usbserial-XXXX
```

Use `/dev/cu.*`, not `/dev/tty.*`, on macOS. Reset the board to see `*** Booting Zephyr OS build v4.3.2 ***`.

## Debug (GDB over the ST-Link)

```bash
west debug -d ~/build --runner openocd
```

Useful GDB commands:

```
break main                      stop at main()
break ws2812_strip_update_rgb   stop in a driver function
continue                        run to the next breakpoint
finish                          run to the end of this function, print its return value
bt                              where am I (after Ctrl-C if it hangs)
x/3xw 0x40013000                SPI1 CR1, CR2, SR
x/xw 0x50000000                 GPIOA MODER (PA1 = bits 3:2; 10 = alternate function)
```

Use `continue` + `finish` to get past driver calls. `next` over a driver call steps hundreds of instructions.

Map a crash or PC address to a function:

```bash
~/zephyr-sdk-0.17.4/arm-zephyr-eabi/bin/arm-zephyr-eabi-addr2line -f -e ~/build/zephyr/zephyr.elf 0x080034f4
```

## Inspect the build

| File / command | Shows |
|---|---|
| `~/build/zephyr/zephyr.dts` | the merged devicetree. Read it, never edit it |
| `~/build/zephyr/.config` | the final Kconfig |
| `~/build/zephyr/include/generated/zephyr/devicetree_generated.h` | the `DT_N_…` macros the C code uses |
| `west build -d ~/build -t rom_report` | flash use by file and function |
| `west build -d ~/build -t ram_report` | RAM use |

Flash is at about 77% (25 KB). `ease_spring()` pulls in soft-float `expf`/`cosf` (~5 KB) when it's used.

## Hardware hookup

| Connector | Pins | To |
|---|---|---|
| J9 | USB-C breakout | power (5 V in) |
| J6 | 1 3V3, 2 SWCLK, 3 GND, 4 SWDIO, 5 NRST | ST-Link. Leave 3V3 unconnected on a clone. NRST is needed (`reset_config srst_only`) |
| J8 | 1 DBG_TX, 2 DBG_RX, 3 GND | serial adapter, TX/RX crossed |
| J1 | 1 GND, 2 LED data (5 V), 3 5V | WS2812 board |
| J5 | 1 CHG_RX, 2 GND, 3 charge 5V | retainer pogo pins |
| J3 / J7 | UV string A / B | keep **unplugged** during bring-up |

`chg_set(true)` puts 5 V on the exposed pogo pins. Never power UV-C with the lid open or look at it.

## Known issues and findings

- **SPI1 needs half-duplex.** In full-duplex mode, the STM32 SPI driver waits for RXNE after each byte. RXNE never sets without an SCK pin, so `led_strip_update_rgb()` hangs (`spi_ll_stm32.c` line 401). `duplex = <SPI_HALF_DUPLEX>` fixes this with no driver change. The `led_fx.c` header comment about SCK on PA1 is out of date.
- **CHG_IMON is only good above ~100 mA.** The TPS259631 current monitor is specified from 0.125 A to 2 A. Expected 2.51 mV/mA (656 µA/A × R18 3.83 kΩ). Current limit ≈ 247 mA ≈ 620 mV. Readings under 5 mA are clamped to 0. Use CHG_RX, not IMON, for "charge complete".
- **LED current shifts the IMON reading.** WS2812 return current through CHG_CASE ground moved the ADC reading by ~25 mV, because the ADC is referenced to VSS (pin 5), not to R18's ground. The code turns the LEDs off for 5 ms before sampling. With the LEDs off, the no-load zero is ~7 mV.
- **Recalibrate `CAL_*` in `main.c`.** The current points (7.43 mA → 30 mV, 15.32 mA → 45 mV) were probably taken with the LEDs lit. Redo them with LEDs off, and add a ~50 mA point (100 Ω, ½ W across J5-3/J5-2).
- **WS2812s keep their last color across MCU resets and reflashes.** Turn them off explicitly, or earlier frames will skew measurements.
- **Shared package pins.** Pin 15 = PB0/PB1/PB2/PA8, pin 20 = PB3–PB6, pin 1 = PB7/PB8. Use only the listed port, and never drive two ports of one pin.
- **CHG_FLT_N has no external pull-up** on the schematic, so the `GPIO_PULL_UP` in the devicetree is required.
- Next board revision: route R18 ground straight to MCU VSS, return LED current separately, and add 10–100 nF on PA12.

## Layout

```
charging-case-firmware/
├── AGENTS.md                  map for AI agents
├── README.md                  this file
├── CMakeLists.txt             app sources (list every .c here)
├── prj.conf                   app Kconfig: GPIO, LED_STRIP, ADC
├── boards/cicada/chg_case/    the custom CHG_CASE board (required)
└── src/
    ├── main.c                 charge enable + CHG_IMON readout + LED demo loop
    ├── led_fx.c/.h            WS2812 effects: bounce(), update_color(), easing curves
    └── chg_ctrl.c/.h          empty stubs (charge module, to be filled)
```

`README.rst` and `sample.yaml` are leftovers from Zephyr's blinky sample and can be deleted.
