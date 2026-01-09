# Система обработки MQTT сообщений в SIM800Handler

## Обзор

Новая система обеспечивает полную обработку входящих MQTT сообщений через SIM800 модуль с использованием AT команд. Система состоит из двух основных компонентов:

1. **SIM800Handler** - низкоуровневая обработка AT команд и парсинг сообщений
2. **MQTTHandler** - высокоуровневая обработка команд и интеграция с бизнес-логикой

## Архитектура

```
┌─────────────────────────────────────────────────────────────┐
│                     Внешний MQTT Брокер                      │
└──────────────────────────────┬──────────────────────────────┘
                               │ MQTT over TCP
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                        SIM800 Модуль                         │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ AT+CMQTTRX: 0,"topic","payload"                     │    │
│  │ +CMQTTRXSTART / +CMQTTRXEND                         │    │
│  └─────────────────────────────────────────────────────┘    │
└──────────────────────────────┬──────────────────────────────┘
                               │ UART (AT Commands)
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                      SIM800Handler                          │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │   Парсер    │  │    Очередь   │  │     Callback     │   │
│  │  сообщений  │──▶   сообщений  │──▶   обработчика    │   │
│  └─────────────┘  └──────────────┘  └──────────────────┘   │
└──────────────────────────────┬──────────────────────────────┘
                               │ API вызовы
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                      MQTTHandler                            │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │  Парсер     │  │    Очередь   │  │  Обработчик      │   │
│  │   JSON      │──▶   команд     │──▶   бизнес-логики  │   │
│  └─────────────┘  └──────────────┘  └──────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## SIM800Handler - Детали реализации

### Обработка входящих сообщений

SIM800 модуль отправляет MQTT сообщения в двух форматах:

#### 1. Компактный формат (одна строка):
```
+CMQTTRX: 0,"topic","payload"
```

#### 2. Расширенный формат (несколько строк):
```
+CMQTTRXSTART: <length>
<topic>
<payload>
+CMQTTRXEND
```

### Методы SIM800Handler

```cpp
// Установка callback для обработки сообщений
void setMessageCallback(MQTTMessageCallback callback);

// Проверка наличия сообщений в очереди
bool hasMessages();

// Получение следующего сообщения из очереди
MQTTMessage getNextMessage();

// Основной цикл обработки (должен вызываться часто)
void mqttLoop();

// Обработка входящих данных из UART
void processIncomingData();
```

### Пример использования

```cpp
SIM800Handler sim800;
SoftwareSerial sim800Serial(2, 3); // RX, TX

void setup() {
    sim800.begin(sim800Serial, 4, 5); // powerPin, resetPin
    
    // Установка callback
    sim800.setMessageCallback([](const String& topic, const String& payload) {
        Serial.print("Message received: ");
        Serial.print(topic);
        Serial.print(" -> ");
        Serial.println(payload);
    });
}

void loop() {
    // Обработка входящих сообщений
    sim800.mqttLoop();
    
    // Или ручная проверка
    while (sim800.hasMessages()) {
        MQTTMessage msg = sim800.getNextMessage();
        // Обработка сообщения
    }
}
```

## MQTTHandler - Детали реализации

### Обработка JSON команд

MQTTHandler ожидает команды в формате JSON:
```json
{
    "address": 256,
    "value": 25.5,
    "type": "holding",
    "command_id": "cmd123",
    "client_id": "optional"
}
```

### Поддерживаемые типы регистров

| Тип в JSON | RegisterType | Описание |
|------------|--------------|----------|
| "coil"     | REG_COIL     | Coil (дискретный выход) |
| "discrete" | REG_DISCRETE | Discrete Input (дискретный вход) |
| "input"    | REG_INPUT    | Input Register (аналоговый вход) |
| "holding"  | REG_HOLDING  | Holding Register (аналоговый ввод/вывод) |

### Методы MQTTHandler

```cpp
// Инициализация с SIM800Handler
void begin(SIM800Handler *sim800);

// Основной цикл обработки
void loop();

// Обработка сообщений из очереди
void processMessages();

// Проверка наличия команд
bool hasPendingCommands();

// Получение следующей команды
MQTTCommand getNextCommand();

// Отправка ответа на команду
void sendCommandResponse(const MQTTCommand &cmd, bool success, const char *message);
```

## Интеграция с существующей системой

### Main.cpp изменения

Основной цикл в `handleRunningState()` уже содержит необходимые вызовы:

```cpp
void handleRunningState(unsigned long currentMillis) {
    // Поддерживаем MQTT соединение
    mqttHandler.loop();
    
    // Обрабатываем MQTT сообщения (команды)
    mqttHandler.processMessages();
    
    // Обрабатываем команды из очереди
    if (mqttHandler.hasPendingCommands()) {
        MQTTCommand cmd = mqttHandler.getNextCommand();
        // ... обработка команды
    }
}
```

## Отладка и мониторинг

### Включение отладки

Добавьте в `setup()`:
```cpp
Serial.begin(115200);
Serial.println("Starting MQTT message handler...");
```

### Мониторинг сообщений

Система логирует:
1. Все входящие AT ответы от SIM800
2. Распарсенные MQTT сообщения
3. JSON команды
4. Ошибки парсинга

### Пример лога
```
SIM800 Response: +CMQTTRX: 0,"modbus/gateway/command","{"address":256,"value":25.5}"
MQTT Message queued: modbus/gateway/command -> {"address":256,"value":25.5}
Command parsed from topic: modbus/gateway/command
Command queued: Address=0x100, Value=25.5
```

## Обработка ошибок

### Типичные ошибки и решения

1. **SIM800 не отвечает на AT команды**
   - Проверьте питание (3.7-4.2V)
   - Проверьте подключение RX/TX
   - Убедитесь, что скорость UART 9600

2. **MQTT сообщения не приходят**
   - Проверьте подписку `mqttHandler.subscribeToCommands()`
   - Убедитесь, что MQTT подключен `sim800Handler.isMQTTConnected()`
   - Проверьте топик `mqttHandler.getCommandTopic()`

3. **JSON команды не парсятся**
   - Проверьте формат JSON
   - Убедитесь, что есть поля `address` и `value`
   - Проверьте размер буфера JSON (увеличьте при необходимости)

## Производительность и оптимизация

### Размер очередей
- SIM800Handler: очередь сообщений (по умолчанию неограничена)
- MQTTHandler: очередь команд (по умолчанию неограничена)

### Потребление памяти
- Каждое сообщение: ~100-500 байт
- Каждая команда: ~200 байт
- Рекомендуется ограничить очереди при работе с большим объемом сообщений

### Частота вызова
- `sim800Handler.mqttLoop()` - вызывать как можно чаще (в основном цикле)
- `mqttHandler.processMessages()` - вызывать периодически (например, каждые 100мс)

## Расширение системы

### Добавление новых типов сообщений

1. Добавьте новый тип сообщения в `MQTTMessage`
2. Реализуйте парсинг в `parseIncomingData()`
3. Добавьте обработку в `processMessages()`

### Кастомная обработка команд

Переопределите обработку в `onMessageReceived()`:
```cpp
void customMessageHandler(const String& topic, const String& payload) {
    if (topic == "custom/topic") {
        // Кастомная обработка
    } else {
        // Стандартная обработка
        onMessageReceived(topic, payload);
    }
}
```

## Заключение

Новая система обеспечивает:
✅ Полную обработку MQTT сообщений через SIM800
✅ Автоматический парсинг JSON команд
✅ Интеграцию с существующей бизнес-логикой
✅ Надежную обработку ошибок
✅ Легкую расширяемость

Система готова к использованию в промышленных условиях с поддержкой отказоустойчивости и мониторинга.