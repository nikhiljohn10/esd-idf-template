# ESP32 ESP-IDF PlatformIO Template

A ready-to-use [PlatformIO](https://platformio.org) project template for ESP32 development
with [ESP-IDF](https://docs.espressif.com/projects/esp-idf/), featuring a collection of
reusable C libraries, Unity unit tests, and a clean project structure.

## Features

- **PlatformIO + ESP-IDF** build system targeting ESP32
- 10 reusable libraries (ADC, BME280, dict, HTTP, JSON, LED, net, OLED, utils, water_level)
- **Unity** unit tests for hardware-independent code
- Thread-safe `dict_t` hash map backed by FreeRTOS stripe locks
- HTTP client with TLS support (`esp_http_client` wrapper)
- SSD1306 128×32 OLED driver with scrolling text, bitmap rendering, and drawing primitives
- Safe credential handling via a gitignored `config.h`

---

## Quick Start

### Prerequisites

- [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) (VS Code extension)
  or [PlatformIO CLI](https://docs.platformio.org/en/latest/core/index.html)
- ESP32 development board (tested on `esp32dev`)

### Setup

1. **Use this template** on GitHub, or clone it directly:

   ```sh
   git clone https://github.com/your-username/esp32-espidf-template.git my-project
   cd my-project
   ```

2. **Copy the example config** and fill in your values:

   ```sh
   cp include/config.h.example include/config.h
   # Edit include/config.h with your WiFi SSID and password
   ```

3. **Build and flash:**

   ```sh
   pio run --target upload
   ```

4. **Monitor serial output:**

   ```sh
   pio device monitor
   ```

---

## Project Structure

```
├── include/
│   ├── config.h.example      # Copy to config.h and fill in your values
│   └── config.h              # (gitignored) Local config with credentials
├── lib/
│   ├── adc/                  # ADC sampling with moving-average
│   ├── bme280/               # BME280 I2C temperature/pressure/humidity driver
│   ├── dict/                 # Thread-safe hash map (FreeRTOS stripe locks)
│   ├── http/                 # HTTP client (all methods, TLS)
│   ├── json/                 # Flat JSON parser and encoder
│   ├── led/                  # GPIO control and LEDC PWM dimming
│   ├── net/                  # WiFi STA + SNTP time synchronisation
│   ├── oled/                 # SSD1306 128×32 I2C OLED driver
│   ├── utils/                # MAX, MIN, delay helpers
│   └── water_level/          # Analog water-level sensor with calibration
├── src/
│   └── main.c                # Application entry point — start here
├── test/
│   ├── test_dict/            # 17 Unity tests
│   ├── test_json/            # 13 Unity tests
│   └── test_utils/           # 10 Unity tests
├── CMakeLists.txt
└── platformio.ini
```

---

## Libraries

| Library       | Description                                                                                     | Docs                                       |
| ------------- | ----------------------------------------------------------------------------------------------- | ------------------------------------------ |
| `adc`         | ADC oneshot sampling with configurable moving-average and delay                                 | [docs/adc.md](docs/adc.md)                 |
| `bme280`      | I2C driver for the Bosch BME280 — temperature, pressure (`hPa` + `bar`) and humidity; atmospheric helpers: VPD, dew point, humidex, heat index, enthalpy, absolute humidity | [docs/bme280.md](docs/bme280.md)           |
| `dict`        | Concurrent hash map — 64 buckets, 8 FreeRTOS mutex stripe locks, djb2 hash                      | [docs/dict.md](docs/dict.md)               |
| `http`        | `http_get`, `http_post`, `http_put`, … wrappers over `esp_http_client` with TLS bundle          | [docs/http.md](docs/http.md)               |
| `json`        | Flat JSON object parser/encoder backed by `dict_t`; strings, numbers, bools, escape sequences   | [docs/json.md](docs/json.md)               |
| `led`         | GPIO LED on/off and LEDC 8-bit PWM (5 kHz) dimming; network status blink patterns               | [docs/led.md](docs/led.md)                 |
| `net`         | Non-blocking `net_start()` launches background WiFi + NTP tasks; `is_wifi_connected()`, `is_ntp_synced()`, `get_ntp_time_string()`, `get_ntp_date_string()`, `wait_for_sec()` | [docs/net.md](docs/net.md)                 |
| `oled`        | SSD1306 128×32 over I2C; 8×8 Latin font, scrolling ticker (`set_text_scroll`), bitmap rendering, drawing primitives, animated WiFi icon, auto-dim | [docs/oled.md](docs/oled.md)               |
| `utils`       | `MAX(a,b)`, `MIN(a,b)`, `delay(ms)` (wraps `vTaskDelay`)                                        | [docs/utils.md](docs/utils.md)             |
| `water_level` | Analog water-level sensor on ADC1 with averaging and dry/full calibration to 0–100 %            | [docs/water_level.md](docs/water_level.md) |

## Examples

| Example                                                  | Description                                                                                                                            |
| -------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| [examples/wifi_led_lamp/](examples/wifi_led_lamp/)       | WiFi-connected smart LED lamp — uses all libraries together: ADC light sensing, LEDC dimming, OLED display, WiFi, SNTP, HTTP, and JSON |
| [examples/weather_station/](examples/weather_station/)   | Mini weather station — reads BME280 temperature/pressure/humidity and shows them on the OLED                                           |
| [examples/water_level_tank/](examples/water_level_tank/) | Tank level monitor — analog water sensor on ADC1, OLED readout, low-water LED alarm                                                    |

---

## Running Tests

Tests run on-device over serial using the Unity framework. Each test suite has its own
`app_main()` and is flashed independently by PlatformIO.

**Flash and run all suites:**

```sh
pio test -e esp32dev
```

**Compile only (no upload):**

```sh
pio test -e esp32dev --without-uploading --without-testing
```

| Suite        | Tests | What is covered                                                 |
| ------------ | ----- | --------------------------------------------------------------- |
| `test_utils` | 10    | `MAX` / `MIN` edge cases                                        |
| `test_dict`  | 17    | Create, set/get/delete, count, foreach, null safety, truncation |
| `test_json`  | 13    | Parse (valid/invalid), encode, quote escaping, roundtrip        |

---

## Configuration

Copy `include/config.h.example` to `include/config.h` (gitignored) and set your values:

```c
#define WIFI_SSID     "your-network-name"
#define WIFI_PASSWORD "your-password"
```

The file is excluded from version control so credentials are never committed. See
[`.gitignore`](.gitignore).

---

## Customising the Template

1. **Add a library** — create `lib/<name>/<name>.h` and `lib/<name>/<name>.c`.
   PlatformIO discovers and links libraries automatically.

2. **Add tests** — create `test/test_<name>/test_<name>.c` with its own `app_main()`.
   PlatformIO discovers test directories automatically.

3. **Edit `src/main.c`** — the provided stub creates a simple blink task.
   Replace it with your application logic using the included libraries.

---

## Marking as a GitHub Template

To let others use this as a GitHub template:

1. Push the repository to GitHub.
2. Go to **Settings → General**.
3. Check **Template repository**.

Users can then click **Use this template** to create their own copy.

---

## License

[MIT](LICENSE) © 2026 [nikhiljohn10](https://github.com/nikhiljohn10)
