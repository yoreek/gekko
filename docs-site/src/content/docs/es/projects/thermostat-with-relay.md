---
title: Termostato con relé
description: Construya un control de temperatura seguro con DS18B20, salida de relé y termostato.
sidebar:
  order: 1
---

Este proyecto controla un calentador o enfriador con un DS18B20. Combina
cableado, dependencias, automatización, estado en vivo y comportamiento seguro.

## Resultado

```text
Sensor DS18B20 → termostato → relé → calentador o enfriador
```

## Hardware

- Placa ESP32.
- DS18B20 con resistencia pull-up de 4,7 kΩ entre DATA y 3V3.
- Módulo de relé adecuado para la carga.
- Calentador o enfriador conectado mediante el relé según su documentación de seguridad.

> Nunca conecte una carga de red directamente al ESP32. Use un relé o contactor
> cerrado y apropiado, y respete las normas locales de seguridad eléctrica.

## Grafo y orden de creación

![Grafo del proyecto. Primero cree el bus 1-Wire, el DS18B20 y el interruptor GPIO; después el termostato.](../../../../assets/diagrams/es/thermostat-project-flow.svg)

1. Cree [`onewire_bus`](/gekko/es/reference/devices/onewire-bus/) en el GPIO del sensor.
2. Ejecute **Scan** y cree el
   [`ds18b20_temperature_sensor`](/gekko/es/reference/devices/ds18b20/) encontrado.
3. Cree un [`gpio_switch`](/gekko/es/reference/devices/gpio-switch/) para el
   relé y establezca como estado seguro el estado sin energía de la carga.
4. Pruebe manualmente el relé con el interruptor GPIO.
5. Cree un [`thermostat`](/gekko/es/reference/devices/thermostat/) y elija el
   sensor y el interruptor como dependencias.

No cree el termostato hasta que el sensor y el interruptor estén `ready`.

## Ajustes mínimos

Para un calentador, elija objetivo e histéresis. Con 25,0 °C y 0,5 °C, se
enciende por debajo de 24,5 °C y se apaga a 25,0 °C. El modo de calefacción y
refrigeración invierte la dirección. Defina límites seguros: si falla el sensor,
la salida debe volver a su estado seguro.

## Verificación y problemas habituales

1. Bus, sensor, interruptor y termostato deben mostrar `ready`.
2. Compare la temperatura con un termómetro fiable.
3. Cambie temporalmente el objetivo y confirme el relé en el límite de histéresis.
4. Desconecte el sensor en una prueba segura y confirme el estado seguro.

- **No aparece el sensor:** revise DATA, 3V3/GND y la resistencia de 4,7 kΩ.
- **Relé invertido:** active inversión solo en módulos activos a nivel bajo.
- **No se puede elegir dependencia:** cree un dispositivo compatible y espere `ready`.
