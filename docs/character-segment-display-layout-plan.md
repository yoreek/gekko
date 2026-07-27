# План layouts для LCD и TM1637

## Управление документом

- Последнее обновление: `2026-07-27`
- Общий статус: `IN PROGRESS`
- Порядок реализации: сначала прошивка, затем SPA
- Область: LCD1602, LCD2004, TM1637 4-digit 0.36" с десятичной точкой у каждого разряда
- Миграция существующих LCD-конфигураций: не требуется

Статусы этапов:

- `NOT STARTED` - работы не начинались.
- `IN PROGRESS` - этап выполняется.
- `BLOCKED` - есть зафиксированный внешний блокер.
- `DONE` - выполнены все задачи и критерии готовности этапа.

При изменении статуса необходимо:

1. Обновить сводную таблицу.
2. Отметить выполненные пункты внутри этапа.
3. Записать результат проверок.
4. Добавить запись в журнал изменений в конце документа.

## Сводный статус

| Этап | Содержание | Статус |
| --- | --- | --- |
| P0 | Исследование и архитектурные решения | `DONE` |
| F1 | Общая модель layout в прошивке | `DONE` |
| F2 | Перевод LCD1602/LCD2004 на layout | `IN PROGRESS` |
| F3 | Реализация TM1637 в прошивке | `IN PROGRESS` |
| F4 | Общая проверка прошивки | `NOT STARTED` |
| S1 | Унификация ядра дизайнера SPA | `IN PROGRESS` |
| S2 | Layout-дизайнер для LCD | `NOT STARTED` |
| S3 | Layout-дизайнер для TM1637 | `NOT STARTED` |
| V1 | Финальная интеграционная проверка и документация | `NOT STARTED` |

## Зафиксированные решения

### 1. Классификация дисплеев

Все поддерживаемые экраны используют одну доменную модель `display layout`.
Координаты `x`, `y`, `width`, `height` не считаются пикселями сами по себе.
Единицу координат задает профиль конкретного дисплея:

| Класс дисплея | Единица координат | Размер рабочей области | Виджеты первого этапа |
| --- | --- | --- | --- |
| OLED/TFT | пиксель | физическое разрешение | существующие виджеты |
| LCD1602 | символьная ячейка | 16 x 2 | `Text` |
| LCD2004 | символьная ячейка | 20 x 4 | `Text` |
| TM1637 4-digit | цифровой разряд | 4 x 1 | `Digital` |

Поля `x`, `y`, `width`, `height` остаются общими. Их смысл определяется
`coordinateUnit` из профиля экрана.

### 2. LCD не получает отдельную систему строк

Legacy per-row fields are not the target model. LCD will
использовать тот же layout sidecar, что OLED и TFT.

Для LCD:

- `x` - номер знакоместа;
- `y` - номер строки;
- `width` - число знакомест в области виджета;
- `height` - число строк в области виджета;
- текст обрезается или переносится внутри области по правилам виджета;
- поддерживается только виджет `Text` на первом этапе;
- поворот не поддерживается, `supportedRotations = [0]`.
- аппаратные линии LCD идут как набор `Switch`-deps, один dep на одну линию
  (RS, E, D4-D7, backlight), без привязки к `PortExpander` как к типу backend;
  это та же универсальная роль, которую уже использует `Thermostat`.

### 3. Для LCD миграции нет

Текущие структуры конфигурации LCD содержат строки и относятся к старой модели.
Так как сохраненный LCD-конфиг еще не использовался, автоматическое преобразование
строк в layout не реализуется.

Правила:

- старые persisted-структуры не изменять на месте;
- создать новые versioned-конфиги без legacy per-row fields;
- не добавлять `migrateFrom`, fallback-декодирование или автоматический seed layout;
- старый бинарный LCD-конфиг считается намеренно неподдерживаемым;
- новый layout создается пользователем через REST/SPA после настройки устройства.

Это решение должно выполняться с учетом
[`docs/device-config-versioning.md`](device-config-versioning.md).

### 4. TM1637 является сегментным цифровым дисплеем

TM1637 не является символьным LCD и не должен использовать виджет `Text` как
основное представление. Для него создается виджет `Digital`, который форматирует
значение в конечный набор разрядов и дополнительных сегментов.

Профиль первого модуля:

- идентификатор панели: `four_digit_decimal_036`;
- 4 разряда;
- 1 строка;
- отдельная десятичная точка у каждого разряда;
- яркость `0..7`;
- прямое подключение `CLK` и `DIO`, без I2C/SPI bus dependency.

Архитектура профиля должна позволять позже добавить:

- TM1637 с центральным двоеточием;
- дисплеи другой разрядности;
- панели с другим расположением дополнительных сегментов;
- совместимые сегментные контроллеры с иным транспортом.

### 5. Семантика десятичных точек TM1637

На исследуемом модуле точка является восьмым сегментом каждого разряда. В байте
разряда это маска `0x80`. Точка не занимает отдельное знакоместо.

Пример:

```text
Вход: 12.34
Разряды: [1] [2.] [3] [4]
Байты: segment(1), segment(2) | 0x80, segment(3), segment(4)
```

Правила `Digital`:

- десятичный разделитель прикрепляет точку к предыдущему разряду;
- точка не уменьшает доступные 4 цифры;
- `fixed:N` задает количество цифр после точки;
- статический шаблон также может явно содержать точки;
- если значение не помещается, отображается настраиваемый overflow-паттерн,
  по умолчанию `----`;
- отсутствующее или нечисловое значение отображается missing-паттерном,
  по умолчанию `----`;
- выравнивание числового значения по умолчанию правое;
- точка хранится в промежуточном кадре отдельно от кода основной цифры.

`Digital` использует существующие источники данных и плейсхолдеры. Например:

```text
{{metric:voltage|fixed:2}}
{{metric:temperature|fixed:1}}
```

### 6. Поворот

LCD1602 и LCD2004:

- только `0`;
- программный поворот на 180 градусов не реализуется, так как стандартная таблица
  символов HD44780 не позволяет корректно повернуть произвольный текст.

TM1637:

- конфигурация предусматривает `0` и `180`;
- при `180` меняется порядок разрядов и выполняется таблица преобразования сегментов;
- физическая точка после переворота окажется сверху слева от соответствующей цифры;
- SPA должна показывать фактическое положение точек и предупреждение, а не
  имитировать нижнюю правую точку;
- поддержка `180` считается готовой только после проверки на реальном модуле.

Если аппаратная проверка покажет неприемлемую читаемость, `180` удаляется из
`supportedRotations`, но поле конфигурации и общий профиль остаются пригодными для
других панелей.

### 7. Версии layout

- JSON schema layout повышается с `1` до `2`.
- В enum виджетов резервируется `Digital = 1`, используя существующий пробел между
  `Text = 0` и `Bitmap = 2`.
- Бинарный `recordVersion` меняется только при фактическом изменении бинарной
  структуры sidecar. Добавление значения enum само по себе не требует изменения
  размера записи.
- Загрузка schema `1` для существующих OLED/TFT layouts сохраняется.
- Запись новых или отредактированных layouts выполняется как schema `2`.

### 8. Render surface разделяются по возможностям

`IDisplayRenderSurface` является только абстрактным маркером семейства surfaces.
В нем нет `clear`, `drawText`, `drawDigital`, графических методов и реализаций по
умолчанию.

От него отдельно наследуются три контракта:

| Контракт | Потребители | Допустимые операции |
| --- | --- | --- |
| `IPixelDisplayRenderSurface` | SSD1306, ST7735/ST7789 | очистка, текст, bitmap и pixel shapes |
| `ICharacterDisplayRenderSurface` | LCD1602, LCD2004 | очистка и `Text` |
| `ISegmentDisplayRenderSurface` | TM1637 и будущие segment displays | очистка и `Digital` |

Обязательные ограничения:

- surface не содержит методов, которые backend не способен выполнить;
- в интерфейсах нет default no-op реализаций;
- LCD не объявляет и не реализует `Digital`, bitmap или shapes;
- TM1637 не объявляет и не реализует `Text`, bitmap или shapes;
- pixel surface не объявляет `Digital`, пока для SSD1306/ST7735 не появится
  отдельная реальная реализация этого виджета;
- `displayProfile.supportedWidgetMask` обязан точно совпадать с typed renderer;
- профиль не используется как оправдание вызова неподдерживаемого метода;
- `dynamic_cast`, RTTI и downcast из общего surface в конкретный не применяются.

Общий обход страниц, расчет refresh interval и placeholder binding остаются в
`DisplayLayoutRenderSession`. Передача widget в конкретный surface выполняется
через отдельные typed layout renderers. Поэтому общий session не знает методов
`drawText`, `drawDigital`, `drawRect` и других hardware-oriented операций.

## Целевая архитектура прошивки

### Общий профиль display surface

Новый файл:

- `src/devices/display/DisplayLayoutProfile.h`

Новые типы:

```cpp
enum class DisplayCoordinateUnit : uint8_t {
  Pixel,
  CharacterCell,
  DigitCell,
};

enum class DisplayAuxSegmentMode : uint8_t {
  None,
  PerDigitDecimalPoint,
  CenterColon,
};

struct DisplayLayoutProfile {
  uint8_t width;
  uint8_t height;
  DisplayCoordinateUnit coordinateUnit;
  uint16_t supportedWidgetMask;
  uint8_t supportedRotationMask;
  DisplayAuxSegmentMode auxSegmentMode;
};
```

Ответственность:

- описать логическую геометрию;
- перечислить доступные виджеты;
- ограничить повороты;
- сообщить SPA о типе дополнительных сегментов;
- не содержать транспортных или аппаратных деталей.

### Общая валидация layout

Новые файлы:

- `src/devices/display/DisplayLayoutValidator.h`
- `src/devices/display/DisplayLayoutValidator.cpp`

Новый класс:

- `DisplayLayoutValidator`

Ответственность:

- проверить schema version;
- проверить поддержку типа каждого виджета профилем;
- проверить `x`, `y`, `width`, `height` относительно логического размера;
- запретить нулевой размер;
- проверить display rotation;
- проверить специфические параметры `Text`, `Digital`, графики и bitmap;
- возвращать стабильный код ошибки и путь проблемного поля для REST.

`DisplayLayoutCodec` остается ответственным за сериализацию и структурную
целостность. Профильная валидация не должна дублироваться в REST-адаптерах.

### Форматирование Digital

Новые файлы:

- `src/devices/display/DisplayDigitalFormatter.h`
- `src/devices/display/DisplayDigitalFormatter.cpp`

Новые типы:

- `DisplayDigitalCell`
- `DisplayDigitalFrame`
- `DisplayDigitalFormatter`

Предлагаемый промежуточный формат:

```cpp
struct DisplayDigitalCell {
  char glyph;
  bool decimalPoint;
};

struct DisplayDigitalFrame {
  DisplayDigitalCell cells[8];
  uint8_t count;
};
```

Ответственность:

- получить вычисленную строку плейсхолдера;
- отделить десятичные точки от цифр;
- применить `fixed`, alignment, padding, missing и overflow policy;
- не знать о кодах сегментов TM1637;
- работать без динамического выделения памяти в runtime hot path.

### Иерархия surface и typed renderer

Новые файлы:

- `src/devices/display/render/DisplayRenderSurface.h`
- `src/devices/display/render/DisplayRenderSurface.cpp`
- `src/devices/display/render/PixelDisplayRenderSurface.h`
- `src/devices/display/render/CharacterDisplayRenderSurface.h`
- `src/devices/display/render/SegmentDisplayRenderSurface.h`
- `src/devices/display/render/DisplayLayoutWidgetRenderer.h`
- `src/devices/display/render/DisplayLayoutWidgetEvaluator.h`
- `src/devices/display/render/DisplayLayoutWidgetEvaluator.cpp`
- `src/devices/display/render/PixelDisplayLayoutWidgetRenderer.h`
- `src/devices/display/render/PixelDisplayLayoutWidgetRenderer.cpp`
- `src/devices/display/render/CharacterDisplayLayoutWidgetRenderer.h`
- `src/devices/display/render/CharacterDisplayLayoutWidgetRenderer.cpp`
- `src/devices/display/render/SegmentDisplayLayoutWidgetRenderer.h`
- `src/devices/display/render/SegmentDisplayLayoutWidgetRenderer.cpp`

Новые классы:

```cpp
class IDisplayRenderSurface {
public:
  virtual ~IDisplayRenderSurface() = 0;
};

class IPixelDisplayRenderSurface : public IDisplayRenderSurface {
public:
  virtual void clear(uint16_t color) = 0;
  virtual void drawText(...) = 0;
  virtual void drawBitmap(...) = 0;
  virtual void drawRect(...) = 0;
  virtual void drawLine(...) = 0;
  virtual void drawCircle(...) = 0;
  virtual void drawEllipse(...) = 0;
};

class ICharacterDisplayRenderSurface : public IDisplayRenderSurface {
public:
  virtual void clear() = 0;
  virtual void drawText(...) = 0;
};

class ISegmentDisplayRenderSurface : public IDisplayRenderSurface {
public:
  virtual void clear() = 0;
  virtual void drawDigital(...) = 0;
};
```

`IDisplayLayoutWidgetRenderer` является adapter-контрактом между общим session и
typed surface. Он работает на уровне `clear frame`/`render widget` и не публикует
аппаратные операции конкретного дисплея.

Pure virtual destructor `IDisplayRenderSurface` получает отдельное пустое
определение в `.cpp`. Это делает marker реально абстрактным, не добавляя в него
capabilities.

Реализации:

- `PixelDisplayLayoutWidgetRenderer` принимает только
  `IPixelDisplayRenderSurface`;
- `CharacterDisplayLayoutWidgetRenderer` принимает только
  `ICharacterDisplayRenderSurface`;
- `SegmentDisplayLayoutWidgetRenderer` принимает только
  `ISegmentDisplayRenderSurface`;
- `DisplayLayoutWidgetEvaluator` содержит общую оценку placeholder для `Text` и
  `Digital`, чтобы она не дублировалась между typed renderers.

Typed renderer возвращает явный результат `rendered`, `unsupported` или
`evaluationFailed`. Неподдерживаемый widget не превращается в успешный no-op.

Изменяемые файлы:

- `src/devices/display/DisplayLayoutStore.h`
- `src/devices/display/DisplayLayoutCodec.cpp`
- `src/devices/display/DisplayLayoutRenderer.h`
- `src/devices/display/DisplayLayoutRenderer.cpp`
- `src/devices/display/DisplayDeviceBase.h`
- `src/devices/display/DisplayDeviceBase.cpp`
- `src/devices/display/DisplayTextPlaceholderAst.cpp`

Изменения:

- добавить `DisplayLayoutWidgetType::Digital = 1`;
- добавить параметры `Digital` в JSON и внутреннее представление;
- предоставить профиль через `DisplayDeviceBase`;
- вызывать `DisplayLayoutValidator` при загрузке и сохранении;
- удалить универсальный набор `draw*` из `IDisplayRenderSurface`;
- заменить `DisplayDeviceBase::renderSurface()` на получение
  `IDisplayLayoutWidgetRenderer`;
- оставить `DisplayDeviceBase::renderDisplay()` общей точкой входа для
  `DisplayRenderCoordinator`;
- очищать пустой layout через общий `IDisplayLayoutWidgetRenderer::clearFrame`,
  а не через surface cast;
- передавать typed surface только соответствующему typed renderer;
- вычислять плейсхолдеры для `Text` и `Digital` через общий evaluator;
- оставить в `DisplayLayoutRenderSession` только page selection, refresh cadence,
  AST binding и агрегирование результата;
- не добавлять RTTI, downcast или default no-op методы;
- создавать typed renderer как небольшой embedded/stack adapter без heap
  allocation в render hot path;
- сохранить существующее поведение OLED/TFT;
- временно исключить `Digital` из pixel profile, пока pixel typed renderer
  действительно его не реализует;
- общую логику определения placeholder dependencies применять ко всем
  текстовым шаблонам, а не только к `Text`.

### REST layout API

Изменяемые файлы:

- `src/integrations/common/DisplayDeviceApiAdapter.h`
- `src/integrations/common/DisplayDeviceApiAdapter.cpp`
- `src/integrations/common/TypedDisplayDeviceApiAdapter.h`
- `src/integrations/common/TypedHd44780DeviceApiAdapter.h`

Целевое изменение:

- выделить единый typed layout adapter для OLED/TFT/LCD/TM1637;
- убрать предположение, что у любого layout-дисплея обязательно есть bus в
  `dependencies[0]`;
- разделить hardware dependencies и metric placeholder dependencies;
- сериализовать `displayProfile` в runtime/config response;
- использовать один код импорта, экспорта, проверки и сохранения layout;
- после перевода всех устройств удалить специализированный
  `TypedHd44780DeviceApiAdapter`.

Новый общий adapter должен предоставлять hooks для конкретного устройства:

- разбор аппаратной части конфигурации;
- проверка аппаратных dependencies;
- дополнение runtime JSON;
- получение `DisplayLayoutProfile`;
- применение конфигурации к runtime device.

## Этап P0. Исследование и архитектура

Статус: `DONE`

- [x] P0.1 Исследована текущая модель layout sidecar.
- [x] P0.2 Исследованы `DisplayLayoutRenderer` и render surfaces OLED/TFT.
- [x] P0.3 Исследована текущая реализация LCD1602/LCD2004 через line templates.
- [x] P0.4 Исследованы REST adapters и dependency extraction.
- [x] P0.5 Исследовано ядро SPA display designer.
- [x] P0.6 Определена модель логических координат.
- [x] P0.7 Определена семантика отдельных точек TM1637.
- [x] P0.8 Зафиксировано отсутствие LCD migration.
- [x] P0.9 Составлен поклассовый план реализации.
- [x] P0.10 Повторно исследован render pipeline и зафиксировано разделение
  surface-интерфейсов по принципу Interface Segregation.

Результат:

- общая layout-модель подходит для пиксельных, символьных и сегментных экранов;
- LCD использует `Text`, TM1637 использует `Digital`;
- специфичность дисплея переносится в profile, validator и render surface.

## Этап F1. Общая модель layout в прошивке

Статус: `DONE`

### F1.1 Профиль и capabilities

- [x] Создать `DisplayLayoutProfile.h`.
- [x] Добавить `DisplayCoordinateUnit`.
- [x] Добавить `DisplayAuxSegmentMode`.
- [x] Определить widget и rotation masks.
- [x] Добавить профиль в контракт `DisplayDeviceBase`.
- [x] Описать профили существующих SSD1306 и ST7789 без изменения поведения.

### F1.2 Schema и Digital

- [x] Добавить `DisplayLayoutWidgetType::Digital = 1`.
- [x] Добавить schema `2` с обратным чтением schema `1`.
- [x] Определить JSON-контракт `Digital`.
- [x] Реализовать `DisplayDigitalFormatter`.
- [x] Расширить placeholder AST/dependency scan для `Digital`.
- [x] Не менять размер binary record без необходимости.

Минимальный JSON `Digital`:

```json
{
  "type": "digital",
  "x": 0,
  "y": 0,
  "width": 4,
  "height": 1,
  "text": "{{metric:voltage|fixed:2}}",
  "align": "right",
  "overflow": "----",
  "missing": "----"
}
```

### F1.3 Валидация

- [x] Создать `DisplayLayoutValidator`.
- [x] Перенести проверки bounds из отдельных adapters в validator.
- [x] Проверять список виджетов по профилю.
- [x] Проверять rotation по профилю.
- [x] Добавить стабильные REST validation errors.

### F1.4 Renderer

Typed surface/renderer split реализован:

- общий `IDisplayRenderSurface` стал пустым marker-интерфейсом;
- pixel, character и segment backends разделены по capability-интерфейсам;
- `DisplayLayoutRenderSession` рендерит через typed renderer без no-op методов;
- `Digital` исключён из pixel profile и остаётся только в digit/segment профиле;
- `DisplayDeviceBase` использует typed hooks `clearDisplay()` и
  `renderDisplayFrame()`.

Задачи архитектурной коррекции:

- [x] Вынести пустой marker `IDisplayRenderSurface`.
- [x] Создать `IPixelDisplayRenderSurface`.
- [x] Создать `ICharacterDisplayRenderSurface`.
- [x] Создать `ISegmentDisplayRenderSurface`.
- [x] Создать `IDisplayLayoutWidgetRenderer` без hardware-specific `draw*`.
- [x] Создать `PixelDisplayLayoutWidgetRenderer`.
- [x] Создать `CharacterDisplayLayoutWidgetRenderer`.
- [x] Создать `SegmentDisplayLayoutWidgetRenderer`.
- [x] Вынести общую оценку `Text`/`Digital` в
  `DisplayLayoutWidgetEvaluator`.
- [x] Перевести `DisplayLayoutRenderSession` на
  `IDisplayLayoutWidgetRenderer`.
- [x] Перевести `DisplayDeviceBase` с `renderSurface()` на typed renderer hook.
- [x] Сохранить одну общую точку `DisplayDeviceBase::renderDisplay()` для
  coordinator.
- [x] Реализовать clear пустого layout через typed renderer.
- [x] Удалить default no-op и все пустые surface methods.
- [x] Исключить `Digital` из SSD1306/ST7735 profile до реальной поддержки.
- [x] Перевести `Ssd1306CanvasSurface` на `IPixelDisplayRenderSurface`.
- [x] Перевести `St7735CanvasSurface` на `IPixelDisplayRenderSurface`.
- [x] Сохранить существующий rendering OLED/TFT без визуальных изменений.
- [x] Не использовать RTTI, `dynamic_cast` или небезопасный downcast.
- [x] Не добавлять heap allocation typed renderer в runtime hot path.

### F1.5 REST

- [x] Унифицировать typed layout adapter.
- [x] Разделить hardware и metric dependencies.
- [x] Добавить `displayProfile` в API.
- [x] Перевести SSD1306/ST7789 на общий adapter.
- [x] Покрыть import/export schema `1` и `2`.

### F1.6 Тесты

Изменяемые или новые тесты:

- `test/test_devices/test_display_layout_renderer.cpp`
- `test/test_devices/test_display_surface_contracts.cpp`
- `test/test_devices/test_display_layout_codec.cpp`
- `test/test_devices/test_display_digital_formatter.cpp`
- REST integration tests layout endpoints

Проверить:

- [x] schema `1` продолжает загружаться;
- [x] schema `2` сохраняется;
- [x] enum `Digital = 1` кодируется стабильно;
- [x] profile bounds работают для pixel/cell/digit;
- [x] неподдерживаемый виджет отклоняется;
- [x] `12.34` становится четырьмя cells с DP у второго cell;
- [x] negative, padding, overflow, missing и `fixed:N`;
- [x] placeholder dependency извлекается из `Digital`.
- [x] Заменить универсальный `FakeDisplaySurface` тремя fake surfaces.
- [x] Проверить pixel renderer на `Text`, bitmap и shapes.
- [x] Проверить character renderer только на `Text`.
- [x] Проверить segment renderer только на `Digital`.
- [x] Проверить явную ошибку renderer при несовпадении widget/profile.
- [x] Проверить согласованность widget mask каждого profile с typed renderer.
- [x] Проверить отсутствие heap allocation в общем render hot path.

Критерий готовности F1:

- существующие OLED/TFT тесты проходят;
- общий layout способен валидировать и отрендерить `Digital` только через
  segment typed renderer;
- LCD fake surface компилируется, реализуя только `clear` и `drawText`;
- TM1637 fake surface компилируется, реализуя только `clear` и `drawDigital`;
- pixel fake surface не обязан реализовывать `Digital`;
- ни один concrete surface не содержит пустых методов;
- profile mask и typed renderer проверяются совместными contract tests;
- REST больше не предполагает единственный тип аппаратной зависимости.

## Этап F2. Перевод LCD1602/LCD2004 на layout

Статус: `IN PROGRESS`

### F2.1 Новые versioned-конфиги

Новые или изменяемые файлы:

- `src/devices/display/lcd1602/Lcd1602DeviceConfig.h`
- `src/devices/display/lcd2004/Lcd2004DeviceConfig.h`
- config codec/marker registrations

Новые структуры:

- `Lcd1602DeviceConfigV2`
- `Lcd2004DeviceConfigV2`

Состав:

- базовый device config;
- layout sidecar как целевая модель текста;
- индексы dependency slots для RS, E, D4-D7 и optional backlight;
- каждый указанный dependency slot обязан иметь универсальную роль `Switch`;
- конкретный backend `Switch` не входит в LCD config: это может быть GPIO,
  канал любого расширителя или будущий output backend;
- bus, модель расширителя и его адрес в LCD config отсутствуют;
- без legacy per-row fields;
- одна символьная/разрядная геометрия через `DisplayLayoutProfile`.

Задачи:

- [ ] Оставить V1 неизменным.
- [x] Создать V2.
- [x] Не создавать migration V1 -> V2.
- [x] Удалить legacy per-row fields из целевой модели конфига.
- [x] Перенести текстовые данные в `layout`-sidecar.
- [x] Не создавать layout из старых line templates.
- [x] Перевести активные aliases/adapters/devices на V2.
- [x] Обновить REST create/update/export/import на layout вместо `lineN`.
- [x] Использовать универсальные `Switch` dependencies вместо зависимости от
  конкретного расширителя.
- [x] Переименовать оставшиеся legacy identifiers `expanderChannels` в
  `dependencySlots`, не меняя wire format.

### F2.2 HD44780 layout surface

Целевые файлы:

- новый `src/devices/display/hd44780/Hd44780CharacterSurface.h`
- новый `src/devices/display/hd44780/Hd44780CharacterSurface.cpp`
- изменить `src/devices/display/hd44780/Hd44780CharacterDisplayDeviceBase.h`
- изменить `src/devices/display/hd44780/Hd44780CharacterDisplayDeviceBase.cpp`

Целевые классы:

- `Hd44780CharacterSurface : public ICharacterDisplayRenderSurface`
- `CharacterDisplayLayoutWidgetRenderer`
- `Hd44780DisplayDeviceBase : public DisplayDeviceBase`

Текущее состояние:

- HD44780-рендер уже проходит через общий `DisplayLayoutRenderSession`;
- `Hd44780CharacterSurface` уже выделена как отдельная runtime-surface;
- `Hd44780CharacterDisplayDeviceBase::displayProfile()` уже сообщает `characterCell` профиль;
- строковая модель persisted layout/config больше не seed-ится из line templates.

После перехода старый `Hd44780CharacterDisplayDeviceBase` переименовывается либо
заменяется `Hd44780DisplayDeviceBase`.

`Hd44780CharacterSurface`:

- использует фиксированный framebuffer максимум 20 x 4;
- `clear()` заполняет его пробелами;
- `drawText()` пишет в cell coordinates с clipping/wrapping;
- не объявляет и не реализует `drawDigital()`;
- не объявляет и не реализует графические методы;
- передается только в `CharacterDisplayLayoutWidgetRenderer`;
- flush сравнивает строки с предыдущим кадром;
- в HD44780 отправляются только измененные строки или непрерывные диапазоны;
- в runtime hot path не создаются большие heap-буферы.

Из текущего `Hd44780CharacterSurface` удалить:

- `drawDigital`;
- `drawRect`;
- `drawLine`;
- `drawCircle`;
- `drawEllipse`;
- `drawBitmap`;
- эмуляцию decimal point через соседнее LCD-знакоместо.

### F2.3 LCD runtime

Изменяемые классы:

- `Lcd1602Device`
- `Lcd2004Device`
- `DisplayRenderCoordinator`
- `CharacterDisplayRuntimeBase`

Задачи:

- [x] Наследовать LCD runtime от `DisplayDeviceBase`.
- [x] Удалить хранение line templates из runtime.
- [x] Подключить общий `DisplayLayoutRenderSession`.
- [x] Сохранить cooperative `tick(now)`.
- [x] Сохранить аппаратный HD44780 transport.
- [x] Предоставить profiles `16 x 2` и `20 x 4`.
- [x] Разрешить только `Text` и rotation `0`.
- [x] Сохранить диагностический вывод фактически отрендеренных строк.
- [x] Удалить отдельную ветку `characterDisplayRuntime()` из coordinator.
- [x] Удалить `CharacterDisplayRuntimeBase`, когда не останется потребителей.
- [x] Привязать линии LCD к набору `Switch`-deps.
- [ ] Удалить из runtime/REST имена, ошибочно указывающие на конкретный backend.

### F2.4 LCD REST

Изменяемые классы:

- `Lcd1602DeviceApiAdapter`
- `Lcd2004DeviceApiAdapter`
- общий typed layout adapter

Задачи:

- [x] Удалить legacy per-row fields из LCD REST config.
- [ ] Подключить общие `/layout` endpoint и setup import/export.
- [ ] Разделить hardware `Switch` dependencies и metric dependencies.
- [ ] Возвращать profile `characterCell`, размер и список виджетов.
- [ ] Удалить `TypedHd44780DeviceApiAdapter`.

### F2.5 LCD тесты

Изменяемые тесты:

- `test/test_devices/test_lcd1602.cpp`
- `test/test_devices/test_lcd2004.cpp`
- LCD REST integration tests
- `test/test_devices/test_display_layout_renderer.cpp`

Проверить:

- [ ] `x/y/width/height` интерпретируются как cells;
- [ ] несколько `Text` виджетов размещаются на одной строке;
- [ ] виджет может занимать несколько строк;
- [ ] clipping не пишет за границы;
- [ ] измененные метрики перерисовывают только нужный кадр;
- [ ] LCD отклоняет `Digital`, графику и bitmap;
- [ ] LCD отклоняет rotation `180`;
- [ ] `Hd44780CharacterSurface` реализует только character surface contract;
- [ ] character renderer не имеет пути вызова pixel/segment методов;
- [x] новый config не содержит line fields;
- [x] никакая LCD migration не вызывается.

Выполнено в этой итерации:

- `Lcd1602DeviceConfigV2` и `Lcd2004DeviceConfigV2` больше не содержат line-полей;
- HD44780 runtime больше не seed-ит layout из line templates;
- LCD tests переведены на явный `DisplayLayoutRecordV1` с `Text` widgets;
- `scripts/test.sh` проходит полностью после правки.

Критерий готовности F2:

- LCD1602 и LCD2004 полностью работают через общий layout;
- отдельной runtime-модели строк больше нет;
- REST предоставляет layout до начала работ над SPA.

## Этап F3. Реализация TM1637 в прошивке

Статус: `IN PROGRESS`

### F3.1 Device config и panel profile

Новые файлы:

- `src/devices/display/tm1637/Tm1637DeviceConfig.h`
- `src/devices/display/tm1637/Tm1637DeviceConfig.cpp`
- `src/devices/display/tm1637/Tm1637PanelProfile.h`
- `src/devices/display/tm1637/Tm1637PanelProfile.cpp`

Новые типы:

- `Tm1637DeviceConfigV1`
- `Tm1637PanelKind`
- `Tm1637PanelProfile`

Поля config:

- base device fields;
- `deps` из двух `switch`-линий;
- `panel = four_digit_decimal_036`;
- `brightness = 0..7`;
- `rotation = 0|180`.

Задачи:

- [x] Проверить абстракцию dependency role вместо привязки к GPIO/PortExpander.
- [x] Запретить одинаковые `CLK` и `DIO`-эквиваленты через duplicate `switch` deps.
- [x] Проверить принятый в проекте механизм конфликтов зависимостей.
- [x] Сделать panel profile расширяемым по разрядности и aux segments.
- [x] Не моделировать TM1637 как I2C device.

### F3.2 Тестируемый transport

Новые файлы:

- `src/devices/display/tm1637/Tm1637LineDriver.h`
- `src/devices/display/tm1637/Tm1637LineDriver.cpp`
- `src/devices/display/tm1637/Tm1637Protocol.h`
- `src/devices/display/tm1637/Tm1637Protocol.cpp`

Новые классы:

- `SwitchTm1637LineDriver`
- `Tm1637Protocol`

Ответственность `SwitchTm1637LineDriver`:

- направление и уровень `CLK`;
- направление и уровень `DIO`;
- короткая protocol delay abstraction для native tests.

Ответственность `Tm1637Protocol`:

- start/stop sequence;
- отправка байта LSB first;
- data command `0x40`;
- address command `0xC0`;
- display control `0x88 | brightness`;
- отправка полного кадра из четырех байтов;
- выключение display без блокирующих retry loops.

### F3.3 Segment codec

Новые файлы:

- `src/devices/display/tm1637/Tm1637SegmentCodec.h`
- `src/devices/display/tm1637/Tm1637SegmentCodec.cpp`

Новый класс:

- `Tm1637SegmentCodec`

Ответственность:

- кодирование `0..9`, `-`, пробела и разрешенного ограниченного набора букв;
- применение `decimalPoint` как `segments | 0x80`;
- отображение `DisplayDigitalFrame` в hardware bytes;
- перестановка разрядов при `180`;
- преобразование основных сегментов при `180`;
- отдельная обработка физического положения DP.

### F3.4 Render surface и device

Новые файлы:

- `src/devices/display/tm1637/Tm1637SegmentSurface.h`
- `src/devices/display/tm1637/Tm1637SegmentSurface.cpp`
- `src/devices/display/tm1637/Tm1637Device.h`
- `src/devices/display/tm1637/Tm1637Device.cpp`

Новые классы:

- `Tm1637SegmentSurface : public ISegmentDisplayRenderSurface`
- `Tm1637Device : public DisplayDeviceBase`

`Tm1637SegmentSurface`:

- хранит логический кадр из четырех digit cells;
- объявляет только `clear()` и `drawDigital()`;
- передается только в `SegmentDisplayLayoutWidgetRenderer`;
- применяет `x` и `width` в digit cells;
- позволяет несколько неперекрывающихся `Digital` виджетов;
- не объявляет `Text`, bitmap или shape methods;
- не знает о GPIO protocol;
- передает итоговый кадр в segment codec.

`Tm1637Device`:

- владеет line driver, protocol, codec и surface;
- выполняет initial configure при `begin`;
- отправляет кадр только при изменении данных, яркости или rotation;
- использует общий render session через `SegmentDisplayLayoutWidgetRenderer` и
  `tick(now)`;
- не выполняет длинных ожиданий и повторов в `loop()`;
- поддерживает enable/disable и shutdown display.

### F3.5 REST и регистрация

Новые или изменяемые файлы:

- `src/integrations/rest/tm1637/Tm1637DeviceApiAdapter.h`
- `src/integrations/rest/tm1637/Tm1637DeviceApiAdapter.cpp`
- `src/devices/core/DeviceTypes.cpp`
- `src/integrations/common/DeviceApiAdapter.cpp`
- device factory/registry files
- PlatformIO source/test registrations, если требуются

Новый класс:

- `Tm1637DeviceApiAdapter`

Задачи:

- [x] Назначить новый стабильный device type ID после проверки registry.
- [x] Зарегистрировать type key `tm1637`.
- [x] Реализовать config CRUD.
- [x] Подключить общий layout API.
- [x] Возвращать profile `4 x 1`, `digitCell`, `Digital`.
- [x] Сериализовать panel kind, brightness и rotation.
- [x] Извлекать metric dependencies из `Digital`.
- [x] Не создавать bus dependency.

Предварительный type ID: `33`. Перед реализацией необходимо повторно проверить,
что ID не занят.

### F3.6 Native tests

Новые тесты:

- `test/test_devices/test_tm1637_display.cpp`
- `test/test_devices/test_main.cpp` registration updates
- TM1637 REST integration coverage in native suite

Проверить:

- [x] start/stop и LSB-first byte sequence;
- [x] команды `0x40`, `0xC0`, brightness и display off;
- [x] таблицу сегментов;
- [x] отдельный DP bit каждого разряда;
- [x] `12.34` на всех четырех разрядах;
- [x] отрицательные числа;
- [x] overflow и missing;
- [x] partial widget с `x/width`;
- [x] frame diff не отправляет неизменные данные;
- [x] rotation `180`;
- [x] config validation и REST layout;
- [x] segment surface не имеет методов `Text`, bitmap и shapes`.

### F3.7 Аппаратная проверка

- [ ] Проверить GPIO wiring на реальном модуле.
- [ ] Проверить brightness `0..7`.
- [ ] Проверить точку у каждого из четырех разрядов.
- [ ] Проверить `12.34`, `-1.2`, overflow и missing.
- [ ] Проверить включение после перезапуска.
- [ ] Проверить rotation `180`.
- [ ] Зафиксировать фактическое положение DP после rotation.

Критерий готовности F3:

- устройство создается и настраивается через REST;
- layout с `Digital` отображает числовую метрику;
- каждая из четырех точек управляется независимо;
- protocol и formatter покрыты native tests;
- результат аппаратной проверки записан в этот документ.

## Этап F4. Общая проверка прошивки

Статус: `NOT STARTED`

- [ ] Запустить `.clang-format` для измененных C/C++ файлов.
- [ ] Запустить `scripts/test.sh`.
- [ ] Проверить отсутствие прямых `Serial.print/printf`.
- [ ] Проверить cooperative runtime и использование `tick(now)`.
- [ ] Проверить фиксированные/reused buffers в render hot path.
- [ ] Проверить отсутствие default no-op и unsupported methods во всех surfaces.
- [ ] Проверить отсутствие RTTI/downcast в render pipeline.
- [ ] Проверить соответствие profile mask конкретному typed renderer.
- [ ] Проверить config version markers.
- [ ] Проверить layout REST для OLED, TFT, LCD и TM1637.
- [ ] Проверить размер прошивки и отсутствие неожиданного роста heap usage.
- [ ] Обновить firmware API docs/examples.

Критерий готовности F4:

- все локальные firmware checks проходят;
- OLED/TFT не получили регрессий;
- LCD и TM1637 готовы для подключения SPA.

## Целевая архитектура SPA

### Общая модель display

Текущая модель `BaseDisplay` предполагает pixel raster. Она должна быть разделена
на общую surface-модель и optional raster capability.

Новые или изменяемые файлы:

- изменить `portal-spa/src/models/devices/display/display.ts`
- изменить `portal-spa/src/models/devices/display/profile.ts`
- изменить `portal-spa/src/models/devices/display/layout.ts`
- изменить `portal-spa/src/models/devices/display/layout-normalizer.ts`
- изменить `portal-spa/src/models/devices/display/display-registry.ts`
- новый `portal-spa/src/models/devices/display/display-coordinate-transform.ts`

Целевые типы:

- `DisplaySurfaceModel`
- `RasterDisplayCapability`
- `DisplayCoordinateUnit = 'pixel' | 'characterCell' | 'digitCell'`
- `DisplayWidgetCapability`
- `DigitalWidget`
- `DisplayCoordinateTransform`

`DisplaySurfaceModel` содержит:

- логические width/height;
- coordinate unit;
- supported rotations;
- supported widgets;
- aux segment mode;
- методы нормализации layout, не зависящие от bitmap format.

`RasterDisplayCapability` содержит только свойства OLED/TFT:

- bitmap formats;
- pixel color rules;
- raster image conversion;
- pixel-specific preview behavior.

### Designer capabilities

Designer не должен содержать hardcoded список всех widget types. Toolbar и
inspector строятся из `displayProfile.supportedWidgets`.

Изменяемые компоненты/composables:

- `portal-spa/src/components/devices/display/DisplayDesignerView.vue`
- `portal-spa/src/components/devices/display/DisplayDesignerCanvas.vue`
- `portal-spa/src/components/devices/display/DisplayDesignerInspector.vue`
- `portal-spa/src/components/devices/display/DisplayWidgetPreview.vue`
- `portal-spa/src/composables/useDisplayDesigner.ts`
- `portal-spa/src/components/devices/display/canvas/geometry.ts`
- `portal-spa/src/components/devices/display/canvas/interaction.ts`

Новые компоненты:

- `DisplayCharacterPreview.vue`
- `DisplayDigitalPreview.vue`
- `SevenSegmentDigit.vue`

Правила:

- Canvas остается общим.
- SVG `viewBox` использует логические display units.
- Для cell/digit surface применяется только визуальный scale, layout coordinates
  остаются целыми.
- Drag/resize выполняют snap к целым cells.
- Pixel surface сохраняет текущую точность.
- `Text` preview LCD использует character cells.
- `Digital` preview рисует реальные семь сегментов и отдельный DP.
- Неподдерживаемые controls не скрываются локальным CSS, а не создаются из
  capabilities.
- Используются Vuetify controls и глобальная типографика проекта.
- Не добавляются локальные overrides цвета, font-weight, opacity и border radius.

## Этап S1. Унификация ядра дизайнера SPA

Статус: `NOT STARTED`

### S1.1 Models и contracts

- [ ] Ввести `DisplaySurfaceModel`.
- [ ] Вынести `RasterDisplayCapability`.
- [x] Добавить coordinate unit и aux segment mode.
- [x] Добавить `DigitalWidget`.
- [ ] Поддержать layout schema `1` и `2`.
- [ ] Удалить fallback неизвестного display к SSD1306.
- [ ] Получать profile из firmware API либо проверенного registry fallback.

### S1.2 Geometry

- [x] Удалить SSD1306-specific types из `geometry.ts`.
- [x] Удалить SSD1306-specific types из `interaction.ts`.
- [ ] Ввести `DisplayCoordinateTransform`.
- [ ] Реализовать snap для character/digit cells.
- [ ] Заменить hardcoded spawn offsets `8/4` на profile-aware offsets.
- [ ] Проверить rotation transforms.

### S1.3 Designer UI

- [x] Формировать toolbar из supported widgets.
- [ ] Формировать inspector sections из типа widget и capabilities.
- [ ] Не показывать bitmap controls без raster capability.
- [x] Добавить общий preview dispatch.
- [x] Унифицировать статический preview карточек дисплеев и More Info через общий компонент.
- [x] LCD1602/LCD2004/TM1637/SSD1306/ST7735 используют один preview contract.
- [ ] Сохранить существующий OLED/TFT UX.
- [x] Добавить unit labels: `px`, `cell`, `digit`.

### S1.4 Unit tests

Изменяемые тесты:

- `portal-spa/tests/unit/models/devices/display-layout.spec.ts`
- `portal-spa/tests/unit/models/devices/display-device-contracts.spec.ts`
- `portal-spa/tests/unit/models/devices/display-display.spec.ts`
- `portal-spa/tests/unit/models/devices/display-orientation.spec.ts`
- designer component/composable tests

Проверить:

- [ ] normalization pixel/cell/digit;
- [ ] schema `1` и `2`;
- [ ] toolbar capabilities;
- [ ] snap и bounds;
- [ ] отсутствие SSD fallback;
- [ ] отсутствие регрессий OLED/TFT.

Критерий готовности S1:

- designer работает с абстрактными logical units;
- raster-specific API не обязателен для LCD/TM1637;
- существующие pixel displays продолжают работать.

## Этап S2. Layout-дизайнер для LCD

Статус: `NOT STARTED`

### S2.1 Models и registry

Изменяемые файлы:

- `portal-spa/src/models/devices/lcd1602.ts`
- `portal-spa/src/models/devices/lcd2004.ts`
- `portal-spa/src/models/devices/display/display-registry.ts`
- `portal-spa/src/components/devices/registry/device-ui-registry.ts`

Задачи:

- [ ] Удалить line fields из новых LCD config models.
- [ ] Зарегистрировать profiles `16 x 2` и `20 x 4`.
- [ ] Разрешить designer action для LCD.
- [ ] Подключить общий layout load/save.

### S2.2 Device forms

Изменяемые компоненты:

- `portal-spa/src/components/devices/lcd1602/Lcd1602Fields.vue`
- `portal-spa/src/components/devices/lcd2004/Lcd2004Fields.vue`

Задачи:

- [ ] Удалить legacy per-row inputs.
- [ ] Оставить только hardware config.
- [ ] Использовать стандартный action `Design display`.
- [ ] Не дублировать редактор layout внутри device dialog.

### S2.3 Character preview

Новый компонент:

- `DisplayCharacterPreview.vue`

Поведение:

- сетка 16 x 2 или 20 x 4;
- monospace cell preview;
- координаты и размеры в cells;
- preview плейсхолдеров;
- clipping/wrapping совпадает с firmware;
- rotation control отсутствует.

### S2.4 Тесты

- [ ] LCD config form не содержит line fields.
- [ ] LCD1602 designer имеет canvas 16 x 2.
- [ ] LCD2004 designer имеет canvas 20 x 4.
- [ ] Toolbar предлагает только `Text`.
- [ ] Inspector использует cell units.
- [ ] Layout round-trip совпадает с REST contract.

Критерий готовности S2:

- LCD layout можно создать, отредактировать и сохранить тем же designer flow, что
  OLED/TFT;
- старой формы строк в SPA больше нет.

## Этап S3. Layout-дизайнер для TM1637

Статус: `NOT STARTED`

### S3.1 Device model и form

Новые файлы:

- `portal-spa/src/models/devices/tm1637.ts`
- `portal-spa/src/components/devices/tm1637/Tm1637Fields.vue`
- при необходимости `Tm1637Device.vue` по существующему registry contract

Изменяемые файлы:

- `portal-spa/src/models/device-type-ids.ts`
- `portal-spa/src/components/devices/registry/device-ui-registry.ts`
- display registry
- i18n message files

Задачи:

- [ ] Добавить type ID после подтверждения firmware registry.
- [ ] Добавить поля `CLK`, `DIO`, panel, brightness, rotation.
- [ ] Зарегистрировать display profile `4 x 1 digitCell`.
- [ ] Подключить standard designer action.

### S3.2 Digital inspector

Изменяемый компонент:

- `DisplayDesignerInspector.vue`

Controls `Digital`:

- placeholder/static template;
- alignment;
- decimal format через существующий `fixed:N`;
- missing pattern;
- overflow pattern;
- preview sample value;
- `x` и `width` в digit cells.

Не добавлять отдельное поле `decimalPosition`, если позиция уже однозначно
определяется результатом formatter. Это предотвращает конфликт между
`fixed:N`, шаблоном и ручной позицией точки.

### S3.3 Seven-segment preview

Новые компоненты:

- `DisplayDigitalPreview.vue`
- `SevenSegmentDigit.vue`

Поведение:

- рисовать семь основных сегментов;
- рисовать отдельную точку каждого разряда;
- показывать активные и неактивные сегменты через Vuetify theme tokens;
- точно показывать `12.34`;
- при rotation `180` показывать реальное перемещение DP;
- выводить предупреждение о положении DP при `180`;
- учитывать brightness только как preview intensity, без изменения layout.

### S3.4 Тесты

- [ ] Form validation GPIO и brightness.
- [ ] Toolbar предлагает только `Digital`.
- [ ] Canvas имеет 4 x 1 digit cells.
- [ ] `12.34` включает DP второго разряда.
- [ ] Каждая DP может быть показана независимо.
- [ ] Overflow/missing preview.
- [ ] Rotation `180` показывает физическое положение DP.
- [ ] Layout round-trip совпадает с firmware.

Критерий готовности S3:

- пользователь может выбрать числовую метрику, формат и увидеть точный preview;
- сохраненный layout сразу работает на TM1637 без SPA-specific преобразования.

## Этап V1. Финальная проверка

Статус: `NOT STARTED`

### Проверки прошивки

- [ ] `scripts/test.sh`
- [ ] Native tests LCD/TM1637/layout.
- [ ] Build целевого ESP32 environment.
- [ ] Проверка реального LCD1602 или LCD2004.
- [ ] Проверка реального TM1637.

### Проверки SPA

- [ ] Unit tests.
- [ ] Type check.
- [ ] Lint/check команды из SPA.
- [ ] Production build и bundle checks.
- [ ] Designer contract tests для всех display profiles.

Browser validation не запускается автоматически. Согласно правилам проекта live
preview server и Playwright-проверка выполняются только после явного запроса или
отдельного разрешения пользователя.

### Документация

- [ ] Обновить REST schema/examples.
- [ ] Обновить список device types.
- [ ] Добавить wiring TM1637.
- [ ] Описать точки и rotation `180`.
- [ ] Обновить документацию display placeholders.
- [ ] Удалить документацию старых LCD line fields.

Критерий готовности V1:

- firmware и SPA checks проходят;
- LCD и TM1637 проверены end-to-end;
- фактические ограничения hardware отражены в документации;
- все этапы в сводной таблице имеют статус `DONE`.

## Матрица ответственности классов

| Класс/модуль | Общая логика | LCD | TM1637 | OLED/TFT |
| --- | --- | --- | --- | --- |
| `DisplayLayoutProfile` | геометрия и capabilities | 16x2/20x4 cells | 4x1 digits, DP | pixels |
| `DisplayLayoutValidator` | bounds/widgets/rotation | используется | используется | используется |
| `IDisplayRenderSurface` | пустой marker base | базовый тип | базовый тип | базовый тип |
| `DisplayLayoutRenderSession` | страницы, refresh, AST binding | используется | используется | используется |
| `DisplayLayoutWidgetEvaluator` | общая оценка placeholders | `Text` | `Digital` | `Text` |
| `CharacterDisplayLayoutWidgetRenderer` | typed dispatch | только `Text` | нет | нет |
| `SegmentDisplayLayoutWidgetRenderer` | typed dispatch | нет | только `Digital` | нет |
| `PixelDisplayLayoutWidgetRenderer` | typed dispatch | нет | нет | `Text`, bitmap, shapes |
| `DisplayDigitalFormatter` | строка -> digit cells | нет | используется | возможно позже |
| `Hd44780CharacterSurface` | cells -> HD44780 buffer | используется | нет | нет |
| `Tm1637SegmentSurface` | widgets -> digit frame | нет | используется | нет |
| `Ssd1306CanvasSurface` | pixel operations | нет | нет | используется |
| `St7735CanvasSurface` | pixel operations | нет | нет | используется |
| `Tm1637SegmentCodec` | digit frame -> segment bytes | нет | используется | нет |
| общий layout REST adapter | config/layout/dependencies | используется | используется | используется |
| `DisplaySurfaceModel` SPA | logical surface contract | используется | используется | используется |
| `DisplayCoordinateTransform` SPA | canvas mapping/snap | cell snap | digit snap | pixel mapping |
| `DisplayCharacterPreview` | символьный preview | используется | нет | нет |
| `DisplayDigitalPreview` | сегментный preview | нет | используется | нет |

## Риски и обязательные проверки

### Риск: общий renderer станет набором backend-specific условий

Мера:

- `DisplayLayoutRenderSession` не знает backend-specific `draw*`;
- пустой `IDisplayRenderSurface` не объявляет capabilities;
- pixel, character и segment operations находятся в разных интерфейсах;
- dispatch разделен между тремя typed renderers;
- hardware conversion находится в concrete surface/codec;
- widget availability определяется profile и validator;
- profile/renderer consistency проверяется contract tests;
- default no-op, RTTI и downcast запрещены.

### Риск: profile объявляет виджет, который surface фактически не рисует

Выявленный пример:

- текущий pixel profile объявляет `Digital`;
- текущие SSD1306/ST7735 surfaces не переопределяют `drawDigital`;
- default no-op скрывает ошибку.

Мера:

- убрать default no-op;
- временно убрать `Digital` из pixel profile;
- возвращать явный `unsupported` из typed renderer при нарушении контракта;
- добавить contract test для каждого profile/renderer pair.

### Риск: координаты cells сломают pixel designer

Мера:

- хранить logical coordinates без пересчета в модели;
- применять scale только в coordinate transform/canvas;
- сохранить отдельные regression tests OLED/TFT.

### Риск: layout schema и config version будут смешаны

Мера:

- config хранит hardware settings;
- layout sidecar хранит widgets;
- версии этих форматов меняются независимо.

### Риск: поворот TM1637 выглядит иначе на физическом модуле

Мера:

- preview показывает физическое положение DP;
- `180` не считается завершенным без аппаратного теста;
- capability можно ограничить до `[0]` без изменения общей архитектуры.

### Риск: TM1637 protocol задерживает cooperative loop

Мера:

- один кадр имеет ограниченный фиксированный размер;
- нет бесконечного ожидания ACK;
- нет retry loop внутри одного `tick`;
- кадр отправляется только при изменении.

### Риск: несколько Digital widgets конфликтуют

Правило первого этапа:

- widgets могут занимать неперекрывающиеся диапазоны digit cells;
- пересечения отклоняются validator для `DigitCell` profile;
- один widget на всю ширину является стандартным сценарием.

## Не входит в первый релиз

- произвольные пользовательские glyphs LCD CGRAM;
- marquee и scrolling text;
- анимация сегментов;
- часы с мигающим центральным двоеточием;
- буквенно-цифровые 14/16-segment displays;
- автоматическая миграция LCD line templates;
- ручное редактирование raw segment bitmask в обычном designer;
- использование TM1637 как универсальной шины для других устройств.

## Журнал изменений

| Дата | Изменение | Статус/результат |
| --- | --- | --- |
| 2026-07-27 | Создан подробный план, зафиксированы архитектура, этапы и классы | P0 `DONE`, остальные этапы `NOT STARTED` |
| 2026-07-27 | Общая модель layout в прошивке реализована и проверена, HD44780-рендер переведен через общий layout pipeline | F1 `DONE`, F2 `IN PROGRESS` |
| 2026-07-27 | LCD1602/LCD2004 переведены на hardware-only config и layout sidecar без legacy per-row fields; native tests проходят | F2 частично `DONE`, SPA/TM1637 остаются |
| 2026-07-27 | Повторный аудит выявил нарушение Interface Segregation в универсальном surface и скрытый no-op `Digital` у pixel displays; зафиксирована typed surface/renderer архитектура | F1 возвращен в `IN PROGRESS`, код не изменялся |
| 2026-07-27 | Typed surface/renderer split реализован, pixel profile не объявляет `Digital`, `test_devices` и полный `scripts/test.sh` проходят | F1 `DONE`, F2 `IN PROGRESS` |
| 2026-07-27 | TM1637 добавлен в прошивку и SPA: device model, UI registry, REST adapter, segment surface и mock seed; `scripts/test.sh` проходит | F3 `IN PROGRESS`, F4 pending |
| 2026-07-27 | SPA display cards унифицированы статическим preview для LCD1602/LCD2004/TM1637/SSD1306/ST7735, More Info dialog переиспользует тот же widget contract; `scripts/test.sh` проходит | S1 `IN PROGRESS` |
| 2026-07-27 | SPA layout designer добран до рабочего состояния для `digital`/`cell` units: toolbar зависит от supported widgets, LCD/TM mock layouts заведены, `DisplayWidget` preview и registry единообразны, `scripts/test.sh` проходит | S1 `IN PROGRESS` |
| 2026-07-27 | Firmware runtime cleanup: `DisplayRenderCoordinator` now renders displays through `displayRuntime()` only, removing the dead `characterDisplayRuntime()` branch; `scripts/test.sh` passes again | F2 `IN PROGRESS` |
| 2026-07-27 | Legacy HD44780 dependency hook renamed from `expanderChannels()` to `dependencySlots()`; port-expander validation still works and `scripts/test.sh` passes | F2 `IN PROGRESS` |
