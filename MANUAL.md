# Nexus Manual

Full command reference for every mode and submode currently in Nexus.

---

## CLI Basics

Nexus uses a layered prompt that reflects where you are:

```
nexus>              global — no mode active
nexus/dio>          inside a mode
nexus/i2c>          inside a mode
```

**Global commands** — available everywhere:

| Command       | Description                          |
|----------------|---------------------------------------|
| `mode <name>`  | Switch to a mode (`dio`, `i2c`, `uart`, `spi`) |
| `help`         | Show help for the current context     |
| `exit`         | Go back one level (mode → global)     |

Most interactive prompts inside a submode also accept `exit`, `cancel`, or `abort` to back out without completing the action.

---

## DIO Mode

Digital I/O tools — interact with GPIO pins without writing any code.

| Submode    | Description                                  |
|------------|-----------------------------------------------|
| `read`     | Read HIGH/LOW state of a pin                 |
| `set`      | Drive a pin HIGH or LOW                       |
| `config`   | Set pin mode — INPUT or OUTPUT                |
| `pwm`      | Generate a PWM signal on a pin                |
| `scan`     | Read all valid GPIO pins and print their state |
| `sniff`    | Monitor a pin continuously for state changes  |
| `toggle`   | Toggle a pin at a fixed interval              |
| `pulse`    | Send a single timed HIGH pulse                |
| `measure`  | Measure frequency on a pin                    |
| `pullup`   | Enable the internal pull-up resistor          |
| `pulldown` | Enable the internal pull-down resistor        |
| `reset`    | Return a pin to its default state             |
| `pins`     | Print the ESP32 DevKit GPIO reference table   |

**Pin safety:** restricted pins (GPIO 6–11, GPIO 0, GPIO 1, GPIO 3, and input-only GPIO 34–39) trigger a warning and a `y/n` confirmation before proceeding. Invalid pin numbers are rejected automatically.

`sniff`, `toggle`, and `measure` run continuously and stop on any keypress.

---

## I2C Mode

Tools for scanning, reading, and debugging I2C devices and buses.

| Submode    | Description                                       |
|------------|----------------------------------------------------|
| `scan`     | Sweep the bus and list responding device addresses |
| `config`   | Set SDA/SCL pins and clock speed                   |
| `save`     | Persist the current pin/speed config to flash      |
| `read`     | Read N bytes from a register at a device address   |
| `write`    | Write a byte to a register at a device address      |
| `dump`     | Read and print all registers (0x00–0xFF) of a device |
| `monitor`  | Poll a register and print only when its value changes |
| `recover`  | Toggle SCL to free a bus stuck with SDA held low    |
| `sniff`    | Passively monitor traffic between two other devices on the bus |

`config` applies settings for the current session only — use `save` to persist them across reboots.

`monitor` and `sniff` run continuously and stop on any keypress.

---

## UART Mode

Tools for talking to, bridging, and probing UART devices.

| Submode   | Description                                          |
|-----------|--------------------------------------------------------|
| `config`  | Set TX/RX pins, baud rate, parity, and stop bits       |
| `save`    | Persist the current UART config to flash               |
| `scan`    | Test common baud rates and report which produce readable data |
| `bridge`  | Bidirectional passthrough between USB serial and UART2 |
| `read`    | Read incoming bytes and print a hex + ASCII dump        |
| `write`   | Send text or raw hex bytes out on UART                  |
| `ping`    | Send a single byte and check for any response           |

`config` applies settings for the current session only — use `save` to persist them across reboots.

`bridge` relays everything you type directly to the connected device. Type `~exit` followed by Enter to leave bridge mode.

---

## SPI Mode

Foundation for configuring and observing an SPI bus.

| Submode   | Description                              |
|-----------|--------------------------------------------|
| `config`  | Set MOSI, MISO, SCK, CS pins, and clock speed |
| `save`    | Persist the current SPI config to flash    |
| `sniff`   | Passively monitor traffic on the SPI bus   |

`config` applies settings for the current session only — use `save` to persist them across reboots.

SPI mode currently has no chip-specific shells (flash, EEPROM, SD). It's kept as a tested foundation — `config`/`save`/`sniff` are ready to support a chip-specific shell whenever one is actually needed.

`sniff` runs continuously and stops on any keypress.

---

## Notes on Persistence

Any mode with `config` + `save` follows the same pattern: `config` changes apply immediately but only for the current session, while `save` writes the configuration to the ESP32's flash (NVS) so it's restored automatically the next time Nexus boots.

---

## Hardware

Default pin assignments, set in `config.h`:

| Bus  | Pins                                  |
|------|----------------------------------------|
| UART | TX = 17, RX = 16                       |
| I2C  | SDA = 21, SCL = 22                     |
| SPI  | MOSI = 23, MISO = 19, SCK = 18, CS = 5 |
| DIO  | Default test pin = 4                   |

All of the above can be changed at runtime with each mode's `config` submode.
