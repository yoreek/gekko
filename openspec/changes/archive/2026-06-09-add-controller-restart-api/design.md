## Context

Фирмварь уже предоставляет async HTTP portal API и поддерживает контролируемый reboot в OTA flow. Одновременно в проекте есть динамический реестр устройств с отложенной записью в NVS, поэтому любой новый reboot path должен сначала пытаться сбросить pending persistence через `DeviceRegistry::flushNow()`.

Сейчас отдельного endpoint для ручного runtime-restart нет. Это усложняет удалённую эксплуатацию и диагностику, потому что restart приходится делать физически или через OTA-сценарии, которые не предназначены для обычного runtime-control.

## Goals / Non-Goals

**Goals:**
- Добавить отдельный HTTP API endpoint для контролируемого restart контроллера.
- Гарантировать единый safe-restart порядок: flush реестра устройств, затем HTTP-ответ, затем restart.
- Возвращать детерминированные ошибки при невозможности flush перед restart.
- Ограничить доступность endpoint в dev/runtime профиле, чтобы исключить непреднамеренно открытый публичный restart.

**Non-Goals:**
- Добавление полноценной authN/authZ модели для всех portal API.
- Добавление shutdown/deep-sleep/factory-reset endpoint в рамках этого change.
- Изменение формата или стратегии persistence реестра устройств.

## Decisions

1. Добавить новый runtime-control endpoint вместо расширения OTA endpoint.

   Новый endpoint будет отделён от OTA API, чтобы restart для эксплуатации не зависел от логики загрузки прошивки. Это также упрощает клиентам явный вызов restart без OTA-контекста.

   Альтернатива: переиспользовать `/api/ota` для restart без upload. Отклонено, потому что семантика OTA и runtime-control различается и смешение ухудшает API-контракт.

2. Применять тот же safe sequence, что и в OTA reboot: `flushNow()` до restart.

   Перед перезагрузкой endpoint обязан вызвать `DeviceRegistry::flushNow()`. При ошибке flush endpoint возвращает HTTP 500 и не инициирует reboot.

   Альтернатива: restart без flush ради минимальной задержки. Отклонено, так как это может терять accepted delayed/coalesced mutations.

3. Отправлять HTTP-ответ до вызова restart.

   Endpoint должен закрыть клиентское соединение и только потом инициировать restart, чтобы у клиента был наблюдаемый результат операции (`rebooting: true` либо структурированная ошибка).

   Альтернатива: немедленный restart без финального ответа. Отклонено, так как ухудшает управляемость и диагностику API.

4. Включить endpoint под явным флагом runtime-control или development profile.

   Новый endpoint должен быть управляемым build flag и по умолчанию безопасным для baseline-сборок без административной защиты.

   Альтернатива: всегда включённый endpoint. Отклонено, так как это повышает операционный и security-риск.

## Risks / Trade-offs

- [Risk] Endpoint может быть случайно открыт в неподходящем профиле. -> Mitigation: compile-time guard и явное требование в spec о policy-driven доступности.
- [Risk] Flush может занимать время и увеличить латентность restart ответа. -> Mitigation: bounded flush path и явный fail response вместо silent timeout.
- [Risk] Клиент может повторно вызывать restart endpoint во время завершения сессии. -> Mitigation: endpoint отвечает детерминированно и закрывает соединение перед restart.
- [Risk] Непоследовательность поведения между OTA reboot и runtime-control reboot. -> Mitigation: использовать общий порядок действий и одинаковую error mapping модель.

## Migration Plan

1. Добавить новый route/handler в portal runtime-control area.
2. Подключить вызов `DeviceRegistry::flushNow()` перед restart и вернуть структурированные ответы.
3. Ограничить endpoint build flag-ом и задокументировать policy включения.
4. Добавить unit/native tests для success/failure path и маршрутизации.
5. Проверить поведение на устройстве: create delayed mutation -> restart endpoint -> verify persisted state after boot.

Rollback:
- Отключить build flag runtime-control endpoint.
- Удалить route registration без изменения существующего OTA restart flow.

## Open Questions

- Нужен ли подтверждающий токен/nonce в теле запроса до появления общей auth-схемы портала?
- Какой endpoint предпочесть как стабильный: `/api/system/restart` или `/api/runtime/restart`?
- Нужен ли минимальный rate-limit на restart endpoint в рамках firmware-side guard?
