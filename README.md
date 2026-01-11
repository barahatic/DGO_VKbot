# DGO_VKbot

Библиотека для работы с VK API через Long Poll для ESP8266 и ESP32.

## Возможности

- Поддержка Long Poll API VK
- Отправка и получение сообщений
- Синхронизация времени через VK API
- Управление таймзоной
- Поддержка ESP8266 и ESP32

## Установка

1. Скопируйте папку `DGO_VKbot` в папку `lib` вашего проекта PlatformIO
2. Библиотека автоматически подключится при компиляции

## Зависимости

- ArduinoJson (версия 7.x)
- WiFi библиотеки (встроенные для ESP8266/ESP32)

Для примера с DHT11 также потребуется библиотека DHT sensor library.

## Быстрый старт

```cpp
#include <DGO_VKbot.h>

#define WIFI_SSID "your_wifi"
#define WIFI_PASS "your_password"
#define VK_TOKEN "your_token"
#define GROUP_ID "-your_group_id"

DGO_VKbot bot;

void onNewMessage(VkUpdate& update) {
  if (update.type == VK_MESSAGE_NEW) {
    bot.sendMessage("Привет!", update.message.peer_id);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  bot.setToken(VK_TOKEN);
  bot.setGroupId(GROUP_ID);
  bot.attach(onNewMessage);
  bot.begin();
}

void loop() {
  bot.tick();
  delay(100);
}
```

## Фильтрация по ID отправителя

Если нужно, чтобы бот отвечал только определенному пользователю, можно добавить проверку ID отправителя в обработчике сообщений:

```cpp
#define MY_USER_ID 123456789  // ID пользователя VK, которому будет отвечать бот

void onNewMessage(VkUpdate& update) {
  if (update.type == VK_MESSAGE_NEW) {
    // Фильтруем: отвечаем только на сообщения от указанного пользователя
    if (update.message.from_id != MY_USER_ID) {
      Serial.print("Сообщение от другого пользователя (ID: ");
      Serial.print(update.message.from_id);
      Serial.println("), пропускаем");
      return;
    }
    
    // Обрабатываем сообщение
    String text = update.message.text;
    int peer_id = update.message.peer_id;
    
    // Ваша логика обработки
    bot.sendMessage("Сообщение получено!", peer_id);
  }
}
```

**Поля структуры VkUpdate для фильтрации:**
- `update.message.from_id` - ID отправителя сообщения
- `update.message.peer_id` - ID чата (для отправки ответа)
- `update.message.text` - текст сообщения
- `update.message.id` - ID сообщения
- `update.message.date` - время отправки (Unix timestamp)

Если не использовать фильтрацию, бот будет отвечать всем пользователям, которые пишут в группу.

## API

### Основные методы

- `setToken(String token)` - установить токен VK
- `setGroupId(String id)` - установить ID группы (с минусом!)
- `begin()` - запустить бота
- `attach(callback)` - прикрепить обработчик сообщений
- `sendMessage(String text, int peer_id)` - отправить сообщение
- `tick()` - обработать события (вызывать в loop)

### Управление временем

- `setTimezone(int hours)` - установить таймзону (например, 3 для UTC+3)
- `syncTime()` - синхронизировать время с сервером VK
- `getCurrentTimeString()` - получить текущее время как строку
- `getSecondsFromMidnight()` - секунды с начала дня

## Примеры

В папке `examples` находятся три примера:

1. **EchoBot** - простой эхо-бот
2. **LEDControl** - управление светодиодом через команды
3. **DHT11Sensor** - получение данных с датчика DHT11

## Поддержка платформ

Библиотека автоматически определяет платформу:
- ESP8266
- ESP32

Для других платформ будет ошибка компиляции.

## Настройка VK

1. Создайте группу в VK
2. Получите токен группы с правами на сообщения
3. Получите ID группы (с минусом)
4. Настройте Long Poll в настройках группы

## Лицензия

 Apache License Version 2.0
