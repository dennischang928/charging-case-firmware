# AGENTS.md: charging-case firmware map

Zephyr 4.3.2 application for the Cicada charging-case main board (CHG_CASE, STM32G030F6P6). It runs on a custom board definition stored in this repo. Build, flash and debug commands, current status and known issues are in [README.md](README.md).

## Map

| Path | What | More |
|---|---|---|
| `README.md` | Status, build/flash/debug/serial commands, hookup, known issues | n/a |
| `CMakeLists.txt` | App sources. Every `.c` file must be listed here | n/a |
| `prj.conf` | App Kconfig | n/a |
| `boards/cicada/chg_case/` | The CHG_CASE board: devicetree (pins, clocks, SPI, ADC), Kconfig, flash runner | [AGENTS.md](boards/cicada/chg_case/AGENTS.md) |
| `src/main.c` | Charge enable, CHG_IMON readout and calibration, demo loop | n/a |
| `src/led_fx.c/.h` | WS2812 status-LED effects (`bounce()`, `update_color()`) | n/a |
| `src/chg_ctrl.c/.h` | Empty stubs for a charge module | n/a |
| `README.rst`, `sample.yaml` | Leftovers from Zephyr's blinky sample | n/a |

## Outside this folder

| Path | What |
|---|---|
| `~/zephyrproject` | The west workspace (Zephyr 4.3.2 + HALs). Run `west` from here |
| `~/build` | Build output. `zephyr.dts` and `.config` there show what was actually built |
| `CHG_CASE.pdf` (Cicada hardware files, not in this repo) | Ground-truth schematic for every pin and net |
