```
╔═════════════════════════════════════════════╗
║  ███╗   ██╗███████╗██╗  ██╗██╗   ██╗███████╗║
║  ████╗  ██║██╔════╝╚██╗██╔╝██║   ██║██╔════╝║
║  ██╔██╗ ██║█████╗   ╚███╔╝ ██║   ██║███████╗║
║  ██║╚██╗██║██╔══╝   ██╔██╗ ██║   ██║╚════██║║
║  ██║ ╚████║███████╗██╔╝ ██╗╚██████╔╝███████║║
║  ╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝║
║           v1.0 | ESP32 | Serial CLI         ║
╚═════════════════════════════════════════════╝
```

![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange?logo=platformio&logoColor=white)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal?logo=arduino&logoColor=white)
![C++](https://img.shields.io/badge/Language-C%2B%2B-blue?logo=cplusplus&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-blue)
![GitHub](https://img.shields.io/badge/GitHub-tsanthosh1328--coder-181717?logo=github&logoColor=white)

A serial-controlled hardware analysis toolkit for ESP32.  
Plug in, open a serial monitor, and start exploring hardware — no sketches, no flashing, no setup.

Nexus eliminates the write-flash-test loop for hardware exploration. Instead of writing a throwaway sketch every time you want to probe a sensor or debug a protocol, you type a command into a serial monitor and get an instant result.

---

## Modes

| Mode | Submodes |
|------|----------|
| **DIO**  | read, set, config, pwm, scan, sniff, toggle, pulse, measure, pullup, pulldown, reset, pins |
| **I2C**  | scan, config, save, read, write, dump, monitor, recover, sniff |
| **UART** | config, save, scan, bridge, read, write, ping |
| **SPI**  | config, save, sniff |

Full command reference: see [MANUAL.md](MANUAL.md)

---

## Requirements

- ESP32 DevKit (WROOM-32)
- PlatformIO
- Serial monitor (115200 baud)

## Flash

```bash
git clone https://github.com/tsanthosh1328-coder/Nexus.git
cd Nexus
pio run --target upload
```

---

## Built With

- [PlatformIO](https://platformio.org/)
- [ESP32 Arduino Framework](https://github.com/espressif/arduino-esp32)

---

## License

MIT — see [LICENSE](LICENSE) for details.

## Author

Santhosh — EE Student, IIT Bombay  
[github.com/tsanthosh1328-coder](https://github.com/tsanthosh1328-coder)
