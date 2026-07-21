---
title: Analog inputs & multiplexers
description: Reading analog voltages in Gekko — the ESP32's own ADC, the precise ADS1115, and the 16-channel CD74HC4067 multiplexer, all behind one channel type.
sidebar:
  order: 11
---

## Why analog inputs?

Plenty of aquarium and greenhouse sensors don't speak a digital protocol — they
just output a **voltage** that changes with what they measure: an NTC
thermistor, a pH or ORP probe board, a TDS/EC stick, a photoresistor, a soil
moisture pad, a pressure or water-level sensor. To read any of them, something
has to turn that voltage into a number — that's an **ADC** (analog-to-digital
converter).

Gekko separates *where the voltage comes from* (the ADC hardware) from *what
the number means* (a temperature, a pH, a level). This page is about the first
half — the four device types that produce a raw voltage reading. Sensors that
interpret that reading, like the
[NTC thermistor](/gekko/reference/devices/ntc-thermistor/), depend on one of
these and add the maths.

Every analog-input device reports the same thing: a reading in **millivolts**
(the authoritative value) plus a raw ADC code for diagnostics, with a validity
flag. A disconnected or unread input shows as *invalid*, never as a fake zero.

## The three ways to get a reading

| Type | Hardware | Channels | Depends on |
| --- | --- | --- | --- |
| `analog_port_input` | The ESP32's own built-in ADC | 1 (its pin) | — |
| `ads1115_hub` | ADS1115 16-bit I2C ADC | 4 | an [I2C bus](/gekko/reference/devices/i2c-bus/) |
| `cd74hc4067_hub` | CD74HC4067 analog multiplexer | 16 | — (owns GPIO pins) |

Which one fits:

- **`analog_port_input`** — the zero-extra-parts option. The ESP32 already has
  an ADC; this reads one of its pins directly. Fine for a rough reading (a
  photoresistor, a coarse level float) where ±a few percent doesn't matter. The
  on-chip ADC is only 12-bit and mildly non-linear, and half its pins stop
  working the moment Wi-Fi is on (see the caveats below).
- **`ads1115_hub`** — when accuracy matters. The ADS1115 is a 16-bit I2C ADC
  with a programmable gain amplifier, so a small signal (a pH board's output, a
  precise thermistor) is measured cleanly and repeatably. Four channels per
  chip, and up to four chips on one bus (addresses `0x48`–`0x4B`).
- **`cd74hc4067_hub`** — when you need *many* cheap channels. The CD74HC4067 is
  a 16-way analog switch: it connects one of 16 inputs to a single shared pin,
  which you feed into any ADC (the ESP32's own, by default). Sixteen soil-moisture
  pads or float levels off one ADC pin — but they still share the on-chip ADC's
  accuracy and are read one at a time.

## Hubs and channels

The ESP32 port input is standalone — it *is* one reading, so you just create it
and point a sensor at it.

The two multi-channel chips work differently, and mirror the
[port-expander pattern](/gekko/guides/devices-and-dependencies/): the **hub**
device owns the chip and its pins, and each channel you actually use is a
separate **`analog_input_channel`** device that depends on the hub.

![One ADS1115 hub with four channels; two channel devices depend on it and are read by sensors, the hub depends on an I2C bus](../../../../assets/diagrams/analog-input-hub.svg)

So a two-probe ADS1115 setup is three devices: the `ads1115_hub`, and two
`analog_input_channel` devices (channel 0 and channel 1) pointing at it. Each
channel is independently named, enabled, and polled — and each can be read by
its own sensor. There is **one** channel type for both hub kinds: a channel
never names the concrete chip, it just asks the hub for "channel N", so an
`analog_input_channel` works the same whether its hub is an ADS1115 (channels
0–3) or a CD74HC4067 (channels 0–15). The create form bounds the channel number
to whatever hub you actually selected.

A channel device is deliberately small — it just names its hub and channel
number, then samples and reports millivolts:

![Analog input channel settings: hub picker, channel number, oversampling, and the live voltage](../../../../assets/screenshots/device-analog-input-channel.png)

Two channels can't claim the same number on one hub — Gekko rejects the second,
the same way it rejects two port-expander switches on one pin.

## Setting up an ADS1115

1. Create an **[I2C bus](/gekko/reference/devices/i2c-bus/)** on your SDA/SCL
   pins (if you don't have one already), and use **Scan bus** to confirm the
   ADS1115 answers — usually at `0x48`.
2. Create an **`ads1115_hub`**, select that bus, and set its address and gain.
3. For each input you've wired, create an **`analog_input_channel`**, select the
   hub, and pick the channel number (0–3 = the ADS1115's A0–A3).
4. Point a sensor (or just watch the channel's live millivolts) at each channel.

![ADS1115 hub settings: I2C bus, address with scan, gain and data rate](../../../../assets/screenshots/device-ads1115-hub.png)

**Gain** sets the input range and therefore the resolution. Pick the smallest
range that still comfortably covers your signal — a smaller range spreads the
16 bits over fewer volts, so each step is finer:

| Gain | Full-scale range | Use when |
| --- | --- | --- |
| `fsr6144` | ±6.144 V | Never needed at 3.3 V — clips the code range |
| `fsr4096` | ±4.096 V | A signal that can reach the full 3.3 V rail |
| `fsr2048` | ±2.048 V | **Default** — good for most 0–2 V signals |
| `fsr1024` | ±1.024 V | Small signals under ~1 V |
| `fsr0512` | ±0.512 V | |
| `fsr0256` | ±0.256 V | Very small signals |

:::caution[Don't exceed the supply]
The ADS1115 can *represent* up to ±6.144 V in code, but you must never feed a
channel more than the chip's supply voltage (VDD, i.e. 3.3 V here). The gain
setting only chooses how the code range maps onto the volts — it does not
protect the input.
:::

## Setting up a CD74HC4067 multiplexer

The CD74HC4067 needs no bus. It has four **address pins** (S0–S3) that Gekko
drives to select which of the 16 inputs is connected to the shared **SIG** pin,
which you wire to an ADC pin (an ESP32 ADC pin by default):

1. Wire S0–S3 to four GPIOs, SIG to an ADC-capable pin, and optionally EN to a
   GPIO (tie EN to GND if you don't wire it).
2. Create a **`cd74hc4067_hub`**, enter the four select pins, the SIG pin, and
   its attenuation.
3. Create an **`analog_input_channel`** per input, selecting the hub and channel
   0–15.

![CD74HC4067 hub settings: the four S0–S3 select pins, enable pin, signal pin and its attenuation](../../../../assets/screenshots/device-cd74hc4067-hub.png)

Because all 16 channels funnel through one ESP32 ADC pin, they share that ADC's
accuracy and the Wi-Fi pin restriction below — the multiplexer buys you channel
*count*, not precision. Reads are sequential: Gekko flips the address lines,
waits one tick for the mux to settle, then samples, so scanning many channels
is naturally paced rather than instantaneous.

## The ESP32 ADC caveats (port input & CD74HC4067 SIG)

Both `analog_port_input` and the CD74HC4067's SIG pin use the ESP32's on-chip
ADC, which has two things worth knowing:

- **Use ADC1 pins with Wi-Fi.** GPIO **32–39** are ADC1 and keep working while
  Wi-Fi runs; the ADC2 pins do not — Wi-Fi owns ADC2, so a reading there stalls
  or returns garbage. GPIO **34–39 are input-only** (no internal pull-ups),
  which is exactly what you want for a sensor input. The default pin is **34**.
- **Attenuation sets the input range.** The raw ADC only measures up to ~1.1 V;
  attenuation scales larger voltages down into that window. Use the widest
  (`11db`, the default) unless your signal is genuinely small:

  | Attenuation | Usable input range |
  | --- | --- |
  | `0db` | ~0 – 0.95 V |
  | `2_5db` | ~0 – 1.3 V |
  | `6db` | ~0 – 1.75 V |
  | `11db` | ~0 – 3.1 V (**default**, full-range) |

For anything where the exact voltage matters, prefer an ADS1115 — the on-chip
ADC is convenient, not precise.

## Smoothing and reporting

Every input reading is the average of several ADC samples taken back-to-back,
which knocks down noise before the value is ever reported. How often it samples
and how eagerly it pushes updates is configurable per device/channel — the
same reporting-delta idea as the temperature sensors, so a jittery raw signal
doesn't flood the WebSocket or the history charts.

## Configuration

### `analog_port_input`

| Field | Default | Meaning |
| --- | --- | --- |
| `gpioPin` | `34` | The ADC pin to read (use ADC1: 32–39, with Wi-Fi on) |
| `attenuation` | `11db` | Input range — see the table above |
| `adcSamples` | `8` | Samples averaged per reading (1–32) |
| `pollMs` | `1000` | How often to read |
| `reportDeltaMilliVolts` | `10` | Minimum change before a new reading is pushed |
| `reportAlways` | off | Push every poll regardless of the delta |

### `ads1115_hub`

| Field | Default | Meaning |
| --- | --- | --- |
| `i2cAddress` | `0x48` | ADS1115 address (`0x48`–`0x4B` by ADDR pin) |
| `gain` | `fsr2048` | Full-scale range / PGA — see the gain table |
| `dataRateSps` | `128` | Samples per second: `8`–`860`; higher = faster but noisier |

### `cd74hc4067_hub`

| Field | Default | Meaning |
| --- | --- | --- |
| `selectPins` | `[16, 17, 18, 19]` | The four S0–S3 address GPIOs |
| `sigPin` | `34` | ADC pin the shared SIG output feeds (ADC1 with Wi-Fi on) |
| `sigAttenuation` | `11db` | Input range for that ADC pin — see the attenuation table |
| `enablePin` | unused | Optional EN GPIO; leave unset and tie EN to GND |

### `analog_input_channel`

| Field | Default | Meaning |
| --- | --- | --- |
| `channel` | `0` | Which hub channel (0–3 on ADS1115, 0–15 on CD74HC4067) |
| `adcSamples` | `4` | Samples averaged per reading (1–32) |
| `pollMs` | `1000` | How often to read |
| `reportDeltaMilliVolts` | `10` | Minimum change before a new reading is pushed |
| `reportAlways` | off | Push every poll regardless of the delta |

## Where the reading goes

An analog input on its own is just a voltage on the dashboard — useful for a
quick check, but the point is usually to feed a sensor:

- an **[NTC thermistor](/gekko/reference/devices/ntc-thermistor/)** turns the
  reading into a temperature, which can then drive a
  [thermostat](/gekko/reference/devices/thermostat/);
- the millivolts show up in
  [display placeholders](/gekko/guides/displays/) for an OLED/TFT;
- on [MQTT builds](/gekko/guides/mqtt-home-assistant/) each leaf input
  (the port input and each channel) is discoverable in Home Assistant as a
  `voltage` sensor. Hubs aren't — they provide channels, not a reading of their
  own.

Firmware internals:
[`docs/analog-input.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-input.md).
