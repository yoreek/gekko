---
title: Каталог пристроїв
description: Усі 35 типів пристроїв, вбудовані в кожен образ прошивки Gekko.
sidebar:
  order: 1
  label: Каталог пристроїв
---

Кожен образ прошивки Gekko містить усі ці типи пристроїв уже вбудованими —
ви створюєте їхні екземпляри з порталу під час роботи. Типи з посиланням
мають окрему довідкову сторінку; решта коротко описані тут, а докладні
сторінки додаються поступово.

## Шини та інфраструктура

| Тип | Призначення |
| --- | --- |
| [`onewire_bus`](/gekko/uk/reference/devices/onewire-bus/) | Шина 1-Wire на GPIO; батьківський пристрій для DS18B20, зі скануванням пристроїв |
| [`i2c_bus`](/gekko/uk/reference/devices/i2c-bus/) | Шина I2C (SDA/SCL); батьківський пристрій для дисплеїв, HTU21, RTC і port-експандерів |
| [`spi_bus`](/gekko/uk/reference/devices/spi-bus/) | Шина SPI; батьківський пристрій для TFT ST7735 |
| `rtc_ds3231` | Годинник реального часу DS3231 — тримає розклади без NTP |
| `rtc_ds1302` | Трипровідний годинник реального часу DS1302 на GPIO CLK, DAT і RST |
| `dummy` | Плейсхолдер/тестовий пристрій |

## Вимикачі та виходи

| Тип | Призначення |
| --- | --- |
| [`gpio_switch`](/gekko/uk/reference/devices/gpio-switch/) | Вихід on/off на GPIO — реле, MOSFET, світлодіоди |
| [`pcf8574_expander`](/gekko/uk/reference/devices/port-expanders/) / [`pcf8575_expander`](/gekko/uk/reference/devices/port-expanders/) | 8-/16-бітні I2C port-експандери |
| [`port_expander_switch`](/gekko/uk/reference/devices/port-expanders/) | Вимикач на одному піні експандера — ті самі опції, що й GPIO switch |
| [`analog_output`](/gekko/uk/reference/devices/analog-outputs/) | PWM-вихід LEDC (димований світильник, вентилятор, …) |
| [`fade_analog_output`](/gekko/uk/reference/devices/analog-outputs/) | Плавні переходи для аналогового виходу |
| [`scheduled_analog_output`](/gekko/uk/reference/devices/analog-outputs/) | Добова крива рівня для аналогового виходу |
| [`analog_output_composer`](/gekko/uk/reference/devices/analog-outputs/) | Об’єднує аналогові канали в одну люстру — див. [приклад з акваріумним світлом](/gekko/uk/reference/devices/analog-outputs/#робочий-приклад-пятиканальне-акваріумне-світло) |

## Аналогові входи

| Тип | Призначення |
| --- | --- |
| [`analog_port_input`](/gekko/uk/reference/devices/analog-inputs/) | Вимірювання напруги прямо з одного ADC-піна ESP32, без додаткового заліза |
| [`ads1115_hub`](/gekko/uk/reference/devices/analog-inputs/) | ADS1115 16-бітний I2C ADC — 4 точні канали |
| [`cd74hc4067_hub`](/gekko/uk/reference/devices/analog-inputs/) | CD74HC4067 16-канальний аналоговий мультиплексор на одному ADC-піні |
| [`analog_input_channel`](/gekko/uk/reference/devices/analog-inputs/) | Один канал ADS1115 або CD74HC4067 hub’а |

## Датчики

| Тип | Призначення |
| --- | --- |
| [`ds18b20_temperature_sensor`](/gekko/uk/reference/devices/ds18b20/) | Температурний зонд DS18B20 1-Wire |
| [`ntc_thermistor_temperature_sensor`](/gekko/uk/reference/devices/ntc-thermistor/) | NTC-терморезистор на будь-якому аналоговому вході |
| [`htu21`](/gekko/uk/reference/devices/htu21/) | HTU21 I2C-датчик температури та вологості |
| `aht10` | AHT10 I2C-датчик температури та вологості |
| `dht11` | DHT11 датчик температури та вологості на одному GPIO |
| `binary_sensor` | Цифровий вхід — поплавковий вимикач, дверний контакт, датчик протікання |

## Керування та автоматика

| Тип | Призначення |
| --- | --- |
| [`thermostat`](/gekko/uk/reference/devices/thermostat/) | Гістерезисне керування нагрівом/охолодженням: датчик на вході, вимикач на виході |
| [`schedule`](/gekko/uk/reference/devices/schedule/) | Розклади з точністю до хвилини за часом доби/днем тижня |
| `auto_switch` | Керує вимикачем з AND-умов, з ручним override і паузою — див. [Розклади та автоматика](/gekko/uk/guides/schedules-and-automation/) |
| [`dosing_pump`](/gekko/uk/reference/devices/dosing-pump/) | Дозування з калібруванням, розкладом і журналом доз |

## Дисплеї

| Тип | Призначення |
| --- | --- |
| `ssd1306` | I2C OLED із [візуальним дизайнером розкладки](/gekko/uk/guides/displays/) |
| `st7735` | Кольоровий SPI TFT, той самий дизайнер розкладки |
| `lcd1602` | Символьний HD44780 LCD 16 × 2 через вбудований I2C-модуль PCF8574 |
| `lcd2004` | Символьний HD44780 LCD 20 × 4 через вбудований I2C-модуль PCF8574 |
| `lcd1602_pin` | Символьний HD44780 LCD 16 × 2, підключений напряму до GPIO-пінів ESP32 |
| `lcd2004_pin` | Символьний HD44780 LCD 20 × 4, підключений напряму до GPIO-пінів ESP32 |
| `tm1637` | Чотирирозрядний семисегментний дисплей із яскравістю та поворотом на 180° |
