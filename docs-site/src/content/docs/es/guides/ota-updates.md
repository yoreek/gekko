---
title: Actualizaciones OTA
description: "Actualizaciones de firmware over-the-air en Gekko: qué funciona en placas de 4 MB y qué necesita más flash."
sidebar:
  order: 6
---

Gekko tiene dos vías OTA, ambas **desactivadas en la compilación por
defecto**: la placa ESP32 clásica de 4 MB simplemente no tiene margen de flash
suficiente para una partición OTA además del firmware, el portal web y la
configuración.

## Compilación por defecto: actualizar por serie

En el diseño estándar de una sola app, las actualizaciones se hacen mediante
[reflasheo por USB](/gekko/es/getting-started/flashing/). La configuración del
dispositivo se guarda en NVS y sobrevive a un reflasheo de firmware, aunque
conviene tener siempre a mano un
[paquete de copia de seguridad](/gekko/es/guides/backup-restore/) antes de
actualizar.

## Subida OTA con PlatformIO (para desarrollo)

Para placas con suficiente flash y un diseño de partición con OTA, el entorno
`esp32dev_ota` de PlatformIO entrega la misma imagen de firmware por red en
vez de por serie:

```sh
pio run -e esp32dev_ota -t upload
```

Es deliberadamente solo un alias de transporte de subida de `esp32dev`: la
misma imagen, los mismos flags de compilación, para que las entregas por serie
y por OTA sean byte a byte idénticas.

## Web OTA (subida desde el portal)

El firmware compilado con la opción protegida de Web OTA añade una página
**OTA** al portal: subes una imagen de firmware desde el navegador, y el
dispositivo la verifica, la finaliza y reinicia en ella. Las subidas demasiado
grandes o interrumpidas dejan intacto el firmware en ejecución. En las
compilaciones sin la función, el portal simplemente oculta la página OTA.

:::note
Trata Web OTA como una función de desarrollo/avanzada: actívala solo en placas
con margen de flash, según `docs/platformio-environments.md` en el repositorio.
:::
