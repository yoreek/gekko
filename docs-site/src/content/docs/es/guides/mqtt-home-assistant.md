---
title: MQTT y Home Assistant
description: Activa un solo interruptor por dispositivo y aparecerá en Home Assistant - interruptores, sensores y termostatos, controlables desde la interfaz de HA.
sidebar:
  order: 4
---

Gekko habla **Home Assistant MQTT discovery**: conéctalo una vez a tu broker
MQTT y luego publica cualquier dispositivo con un solo interruptor - y
aparecerá en Home Assistant por sí solo, con el tipo de entidad, el nombre y
el icono correctos. Sin YAML, sin configuración manual de entidades.

## Qué obtienes

![Dispositivos Gekko apareciendo en Home Assistant como entidades de switch, sensor y climate](../../../../assets/diagrams/ha-entities.svg)

Cada dispositivo Gekko publicado se convierte en una entidad nativa de HA, y
el control funciona en ambos sentidos en tiempo real:

| Dispositivo Gekko | En Home Assistant | Puedes... |
| --- | --- | --- |
| GPIO / port-expander / auto switch | `switch` | conmutarlo desde cualquier panel de HA y usarlo en automatizaciones |
| Salida analógica (fade / scheduled) | `light` (brillo) | atenuarla desde HA, incluirla en escenas |
| Pixel strip | `light` (brillo) | controlar la alimentación y el brillo de una tira direccionable |
| Pixel effect solid | `light` (RGB) | elegir el color de la tira desde la rueda de color de HA |
| Pixel effect alert | `binary_sensor` | saber desde HA si la alerta está parpadeando |
| DS18B20, termistor NTC | `sensor` | trazar el historial, disparar automatizaciones por temperatura |
| HTU21 | dos `sensor`s (temperatura + humedad) | lo mismo, por separado |
| Sensor binario | `binary_sensor` | recibir alertas de fuga/puerta por notificaciones de HA |
| Termostato | `climate` | cambiar modo y punto de consigna desde la tarjeta de termostato de HA |

Así, la luz de tu acuario puede entrar en escenas de HA, el sensor de fuga
puede mandar una notificación al móvil y el termostato puede aparecer junto a
los controles climáticos de tu casa, mientras todo sigue funcionando
localmente en el ESP32 aunque HA caiga.

## Configuración

1. **Conecta el broker (una vez).** En la página **MQTT / Home Assistant** del
   portal introduce host, puerto y credenciales de tu broker (TLS soportado) y
   activa **Enable MQTT**. Los cambios aplican con una reconexión limpia: no
   hace falta reiniciar. MQTT solo se conecta cuando el dispositivo ya está en
   WiFi como estación, nunca en modo AP de configuración.

   ![Página de ajustes del broker MQTT](../../../../assets/screenshots/portal-mqtt.png)

2. **Asegúrate de que HA usa el mismo broker** con el descubrimiento
   habilitado en su integración MQTT (el valor por defecto).

3. **Publica dispositivos.** Cada página de dispositivo compatible tiene una
   tarjeta **Home Assistant**: activa **Publish to Home Assistant**, si quieres
   dale un nombre específico para HA y guarda:

   ![Tarjeta de Home Assistant por dispositivo con el interruptor de publicación](../../../../assets/screenshots/device-ha-card.png)

   En unos segundos el dispositivo aparecerá en HA bajo **Settings → Devices &
   services → MQTT**, agrupado bajo tu controlador Gekko. Al quitar la
   publicación desaparece igual de limpio.

## Una opción en tiempo de compilación

El soporte MQTT se compila en el firmware solo cuando se necesita
(`-DWITH_HOME_ASSISTANT` en `platformio.ini`), así que el firmware sin él no
lleva código MQTT en absoluto, algo importante en placas de 4 MB. El portal
explica claramente la diferencia:

- la insignia **Available / Not available** en la página MQTT indica si esta
  *compilación* tiene la función;
- el interruptor **Enable MQTT** le dice al firmware si debe conectar ahora
  mismo.

En compilaciones sin la función, la página MQTT muestra una nota explicativa y
las tarjetas HA por dispositivo no se renderizan.

Para ver la arquitectura completa (esquema de topics, adaptadores,
certificados TLS), consulta
[`docs/mqtt-home-assistant.md`](https://github.com/yoreek/gekko/blob/master/docs/mqtt-home-assistant.md)
en el repositorio.
