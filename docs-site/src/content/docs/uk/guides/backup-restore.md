---
title: Резервне копіювання та відновлення
description: Експортуйте повну конфігурацію Gekko як один файл, який можна редагувати вручну, і відновлюйте її будь-де.
sidebar:
  order: 5
---

Уся ваша конфігурація пристроїв — кожен пристрій, його налаштування, граф
залежностей, розкладки дисплеїв і дашборд — експортується як **один файл**,
який можна завантажити, зберегти, відредагувати вручну та відновити на
цьому або іншому контролері.

## Через портал

Відкрийте **System → Backup**:

- **Download** зберігає `device-setup.ndjson`.
- **Restore** завантажує bundle з підтвердженням. Спершу bundle валідується;
  будь-яка помилка відхиляє імпорт **без зміни живої конфігурації**, а
  успішне відновлення атомарно замінює всі пристрої.

## Через командний рядок

```sh
# backup
curl -fsS http://<device-ip>/api/device-setup/export -o device-setup.ndjson

# restore
curl -fsS -F "bundle=@device-setup.ndjson" http://<device-ip>/api/device-setup/import
```

## Bundle — це звичайний JSON, який можна редагувати вручну

Файл має формат NDJSON: один JSON-об’єкт на рядок, у тій самій формі, яку
приймає REST API. Ви можете поправити номер GPIO в текстовому редакторі або
навіть написати мінімальний bundle з нуля:

```json
{"kind":"transfer_envelope","transferSchemaVersion":3}
{"kind":"device","record":{"id":4,"typeName":"gpio_switch"},"config":{"name":"Pump","enabled":true,"gpioPin":26}}
```

Для кожного пристрою потрібні лише `record.id` і `record.typeName` — решта
отримує значення за замовчуванням. Ідентифікатори залежностей посилаються на
інші `record.id` у тому самому bundle, а bundles зі старіших прошивок
імпортуються без проблем, якщо їхні поля все ще парсяться. Повний опис
формату: [`docs/backup-and-restore.md`](https://github.com/yoreek/gekko/blob/master/docs/backup-and-restore.md).

## Що входить і не входить

**Входить:** реєстр пристроїв (усі типи, залежності, розкладки дисплеїв) і
розкладка дашборду.

**Не входить:** WiFi-облікові дані, MQTT-налаштування, retained runtime
state, а також firmware/OTA state.

## Автоматичні резервні копії

У прошивки навмисно немає вбудованого планувальника — endpoint експорту це
звичайний GET, який може забрати будь-яка машина у вашій локальній мережі.
Приклад щоденного cron:

```sh
# /etc/cron.d/gekko-backup — щодня о 03:00, зберігати 30 днів
0 3 * * * user curl -fsS http://192.168.1.240/api/device-setup/export \
  -o /var/backups/gekko/device-setup-$(date +\%F).ndjson \
  && find /var/backups/gekko -name 'device-setup-*.ndjson' -mtime +30 -delete
```

Або Home Assistant `shell_command` + автоматизація на тригері часу. Див. doc
вище для готового прикладу HA.

:::caution
У порталу немає автентифікації — будь-хто у вашій мережі може прочитати
bundle (і сам портал). У конфігах немає WiFi/MQTT-секретів, але тримайте
контролер у мережі, якій довіряєте.
:::
