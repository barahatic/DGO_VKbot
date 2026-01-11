// DGO_VKbot.h - Библиотека для работы с VK API через Long Poll
// Поддержка ESP8266 и ESP32
// Автор: DGO
// Версия: 1.0.0

#ifndef DGO_VKBOT_H
#define DGO_VKBOT_H

#include <Arduino.h>
#include <functional>
#include <time.h>
#include <ArduinoJson.h>

// Поддержка ESP8266 и ESP32
#ifdef ESP8266
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
    #include <WiFiClientSecure.h>
#elif defined(ESP32)
    #include <WiFi.h>
    #include <HTTPClient.h>
    #include <WiFiClientSecure.h>
#else
    #error "Платформа не поддерживается. Используйте ESP8266 или ESP32"
#endif

// Типы событий VK Long Poll
enum VkEventType {
    VK_MESSAGE_NEW,
    VK_MESSAGE_REPLY,
    VK_MESSAGE_EDIT,
    VK_UNKNOWN
};

// Структура сообщения
struct VkMessage {
    int id;
    int from_id;
    int peer_id;
    String text;
    unsigned long date;
    
    VkMessage() : id(0), from_id(0), peer_id(0), date(0) {}
    VkMessage(String t, int p) : id(0), from_id(0), peer_id(p), text(t), date(0) {}
};

// Структура обновления
struct VkUpdate {
    VkEventType type;
    VkMessage message;
    
    VkUpdate() : type(VK_UNKNOWN) {}
};

// Класс VK бота
class DGO_VKbot {
private:
    String token;
    String groupId;
    WiFiClientSecure client;
    
    // Long Poll параметры
    String lpServer;
    String lpKey;
    String lpTs;
    
    // Флаг запуска
    bool started;
    
    // Callback для новых сообщений
    std::function<void(VkUpdate&)> newMessageCallback;
    
    // Управление временем и таймзоной
    time_t systemTime;              // Системное время в UTC
    unsigned long lastTimeUpdate;   // Когда последний раз обновляли время (millis)
    int timezoneOffset;             // Смещение таймзоны в секундах (по умолчанию 0 - UTC)
    
    // Кодирование URL
    String urlEncode(String str) {
        String encoded = "";
        char c;
        for (unsigned int i = 0; i < str.length(); i++) {
            c = str.charAt(i);
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else if (c == ' ') {
                encoded += "%20";
            } else {
                encoded += "%";
                if (c < 16) encoded += "0";
                encoded += String(c, HEX);
            }
        }
        return encoded;
    }
    
    // Получение Long Poll сервера
    bool getLongPollServer() {
        HTTPClient http;
        String url = "https://api.vk.com/method/groups.getLongPollServer?";
        url += "access_token=" + token;
        url += "&group_id=" + groupId.substring(1); // Без минуса
        url += "&v=5.199";
        
        http.begin(client, url);
        http.setTimeout(5000);
        
        if (http.GET() == 200) {
            String response = http.getString();
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, response);
            
            if (!error && doc["response"].is<JsonObject>()) {
                lpServer = doc["response"]["server"].as<String>();
                lpKey = doc["response"]["key"].as<String>();
                lpTs = doc["response"]["ts"].as<String>();
                
                Serial.print("[VK] Long Poll сервер: ");
                Serial.println(lpServer);
                return true;
            } else if (doc["error"].is<JsonObject>()) {
                Serial.print("[VK] API ошибка: ");
                Serial.println(doc["error"]["error_msg"].as<String>());
            }
        }
        
        http.end();
        Serial.println("[VK] Ошибка получения Long Poll сервера");
        return false;
    }
    
    // Обработка Long Poll событий
    bool processLongPoll() {
        HTTPClient http;
        String url = lpServer + "?act=a_check&key=" + lpKey + 
                    "&ts=" + lpTs + "&wait=25&mode=2&version=3";
        
        http.begin(client, url);
        http.setTimeout(30000); // 30 секунд максимум
        
        int httpCode = http.GET();
        
        if (httpCode == 200) {
            String response = http.getString();
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, response);
            
            if (!error) {
                // Обновляем ts для следующего запроса
                if (doc["ts"].is<String>()) {
                    lpTs = doc["ts"].as<String>();
                }
                
                // Проверяем на ошибки Long Poll (failed, pts)
                if (doc["failed"].is<int>()) {
                    int failed = doc["failed"].as<int>();
                    if (failed == 1) {
                        // Нужно обновить ts
                        if (doc["ts"].is<String>()) {
                            lpTs = doc["ts"].as<String>();
                        }
                        http.end();
                        return true;
                    } else if (failed == 2 || failed == 3) {
                        // Нужно переподключиться
                        Serial.println("[VK] Long Poll требует переподключения");
                        http.end();
                        return getLongPollServer();
                    }
                }
                
                // Обрабатываем события
                if (doc["updates"].is<JsonArray>()) {
                    JsonArray updates = doc["updates"].as<JsonArray>();
                    
                    for (JsonObject update : updates) {
                        VkUpdate vkUpdate;
                        String type = update["type"].as<String>();
                        
                        if (type == "message_new") {
                            vkUpdate.type = VK_MESSAGE_NEW;
                            JsonObject msg = update["object"]["message"];
                            vkUpdate.message.id = msg["id"].as<int>();
                            vkUpdate.message.from_id = msg["from_id"].as<int>();
                            vkUpdate.message.peer_id = msg["peer_id"].as<int>();
                            vkUpdate.message.text = msg["text"].as<String>();
                            vkUpdate.message.date = msg["date"].as<unsigned long>();
                            
                            // Вызываем callback если есть
                            if (newMessageCallback) {
                                newMessageCallback(vkUpdate);
                            }
                        }
                    }
                    http.end();
                    return true;
                }
            } else {
                Serial.print("[VK] JSON ошибка: ");
                Serial.println(error.c_str());
            }
        } else if (httpCode == -1) {
            // Таймаут - нормально для Long Poll
            http.end();
            return true;
        } else {
            // Серверная ошибка - переподключаемся
            Serial.print("[VK] Long Poll HTTP ошибка: ");
            Serial.println(httpCode);
            delay(1000);
            http.end();
            return getLongPollServer();
        }
        
        http.end();
        return false;
    }

public:
    // Конструктор
    DGO_VKbot() : started(false), systemTime(0), lastTimeUpdate(0), timezoneOffset(0) {
        client.setInsecure();
    }
    
    // Установить токен
    void setToken(String t) {
        token = t;
    }
    
    // Установить ID группы (с минусом!)
    void setGroupId(String id) {
        groupId = id;
    }
    
    // Запуск бота
    bool begin() {
        if (token.length() == 0 || groupId.length() == 0) {
            Serial.println("[VK] Установите токен и ID группы!");
            return false;
        }
        
        if (!getLongPollServer()) {
            return false;
        }
        
        started = true;
        Serial.println("[VK] Бот запущен с Long Poll");
        return true;
    }
    
    // Прикрепить обработчик сообщений
    void attach(std::function<void(VkUpdate&)> callback) {
        newMessageCallback = callback;
    }
    
    // Отправить сообщение
    bool sendMessage(VkMessage msg) {
        if (!started) {
            Serial.println("[VK] Бот не запущен!");
            return false;
        }
        
        HTTPClient http;
        String url = "https://api.vk.com/method/messages.send?";
        url += "access_token=" + token;
        url += "&peer_id=" + String(msg.peer_id);
        url += "&message=" + urlEncode(msg.text);
        url += "&random_id=" + String(random(1000000));
        url += "&v=5.199";
        
        http.begin(client, url);
        http.setTimeout(5000);
        int httpCode = http.GET();
        
        bool success = false;
        if (httpCode == 200) {
            String response = http.getString();
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, response);
            
            if (!error) {
                if (doc["response"].is<int>()) {
                    success = true;
                } else if (doc["error"].is<JsonObject>()) {
                    Serial.print("[VK] Ошибка отправки: ");
                    Serial.print(doc["error"]["error_code"].as<int>());
                    Serial.print(" - ");
                    Serial.println(doc["error"]["error_msg"].as<String>());
                }
            }
        } else {
            Serial.print("[VK] HTTP ошибка отправки: ");
            Serial.println(httpCode);
        }
        
        http.end();
        return success;
    }
    
    // Быстрая отправка (перегрузка)
    bool sendMessage(String text, int peer_id) {
        VkMessage msg(text, peer_id);
        return sendMessage(msg);
    }
    
    // Получить время сервера VK
    unsigned long getServerTime() {
        if (!started) {
            Serial.println("[VK] Бот не запущен, невозможно получить время");
            return 0;
        }
        
        HTTPClient http;
        String url = "https://api.vk.com/method/utils.getServerTime?";
        url += "access_token=" + token;
        url += "&v=5.199";
        
        http.begin(client, url);
        http.setTimeout(5000);
        int httpCode = http.GET();
        
        unsigned long serverTime = 0;
        if (httpCode == 200) {
            String response = http.getString();
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, response);
            
            if (!error && doc["response"].is<unsigned long>()) {
                serverTime = doc["response"].as<unsigned long>();
            } else if (doc["error"].is<JsonObject>()) {
                Serial.print("[VK] Ошибка получения времени: ");
                Serial.println(doc["error"]["error_msg"].as<String>());
            }
        } else {
            Serial.print("[VK] HTTP ошибка при получении времени: ");
            Serial.println(httpCode);
        }
        
        http.end();
        return serverTime;
    }
    
    // Тикер - обработать события
    void tick() {
        if (started) {
            processLongPoll();
        }
    }
    
    // Проверить подключение
    bool isStarted() {
        return started;
    }
    
    // === УПРАВЛЕНИЕ ВРЕМЕНЕМ И ТАЙМЗОНОЙ ===
    
    // Установить смещение таймзоны в секундах (например, 10800 для UTC+3)
    bool setTimezoneOffset(int offsetSeconds) {
        if (offsetSeconds < -43200 || offsetSeconds > 50400) {
            Serial.print("[VK] Неверное значение таймзоны: ");
            Serial.print(offsetSeconds);
            Serial.println(" секунд (должно быть от -43200 до +50400)");
            return false;
        }
        
        timezoneOffset = offsetSeconds;
        int hours = offsetSeconds / 3600;
        int minutes = (abs(offsetSeconds) % 3600) / 60;
        Serial.print("[VK] Таймзона установлена: UTC");
        if (offsetSeconds >= 0) Serial.print("+");
        Serial.print(hours);
        if (minutes > 0) {
            Serial.print(":");
            if (minutes < 10) Serial.print("0");
            Serial.print(minutes);
        }
        Serial.print(" (");
        Serial.print(offsetSeconds);
        Serial.println(" секунд)");
        return true;
    }
    
    // Установить таймзону в часах (например, 3 для UTC+3)
    bool setTimezone(int offsetHours) {
        if (offsetHours < -12 || offsetHours > 14) {
            Serial.print("[VK] Неверное значение таймзоны: ");
            Serial.print(offsetHours);
            Serial.println(" часов (должно быть от -12 до +14)");
            return false;
        }
        return setTimezoneOffset(offsetHours * 3600);
    }
    
    // Получить текущее смещение таймзоны в секундах
    int getTimezoneOffset() {
        return timezoneOffset;
    }
    
    // Синхронизировать время с сервером VK (UTC)
    bool syncTime() {
        if (!started) {
            Serial.println("[VK] Бот не запущен, невозможно синхронизировать время");
            return false;
        }
        
        Serial.println("[VK] Синхронизация времени с сервером VK...");
        
        unsigned long vkTime = getServerTime();
        if (vkTime == 0) {
            Serial.println("[VK] Ошибка получения времени от VK API");
            return false;
        }
        
        systemTime = vkTime;
        lastTimeUpdate = millis();
        
        struct tm *timeinfo_utc = gmtime(&systemTime);
        if (timeinfo_utc != nullptr) {
            char timeStr[30];
            strftime(timeStr, sizeof(timeStr), "%d.%m.%Y %H:%M:%S UTC", timeinfo_utc);
            Serial.print("[VK] Время синхронизировано: ");
            Serial.print(systemTime);
            Serial.print(" (");
            Serial.print(timeStr);
            Serial.print(")");
            
            if (timezoneOffset != 0) {
                time_t localTime = systemTime + timezoneOffset;
                struct tm *timeinfo_local = gmtime(&localTime);
                if (timeinfo_local != nullptr) {
                    strftime(timeStr, sizeof(timeStr), "%d.%m.%Y %H:%M:%S", timeinfo_local);
                    Serial.print(", локальное: ");
                    Serial.print(timeStr);
                }
            }
            Serial.println();
        }
        
        return true;
    }
    
    // Получить текущее время с учетом таймзоны
    time_t getCurrentTime() {
        if (systemTime == 0) {
            return 0;
        }
        
        unsigned long elapsedSeconds = (millis() - lastTimeUpdate) / 1000;
        time_t currentTime = systemTime + elapsedSeconds + timezoneOffset;
        
        return currentTime;
    }
    
    // Получить текущее время как строку
    String getCurrentTimeString() {
        time_t currentTime = getCurrentTime();
        
        if (currentTime < 946684800) {
            return "Время не синхронизировано";
        }
        
        struct tm *timeinfo = gmtime(&currentTime);
        if (timeinfo == nullptr) {
            return "Ошибка получения времени";
        }
        
        if (timeinfo->tm_hour > 23 || timeinfo->tm_min > 59 || timeinfo->tm_sec > 59) {
            return "Некорректное время";
        }
        
        char timeStr[30];
        strftime(timeStr, sizeof(timeStr), "%d.%m.%Y %H:%M:%S", timeinfo);
        return String(timeStr);
    }
    
    // Получить секунды с начала дня (0-86399)
    unsigned long getSecondsFromMidnight() {
        time_t currentTime = getCurrentTime();
        
        if (currentTime < 946684800) {
            return 0;
        }
        
        struct tm *timeinfo = gmtime(&currentTime);
        if (timeinfo == nullptr) {
            return 0;
        }
        
        if (timeinfo->tm_hour > 23 || timeinfo->tm_min > 59 || timeinfo->tm_sec > 59) {
            return 0;
        }
        
        return timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec;
    }
    
    // Проверить, синхронизировано ли время
    bool isTimeSynced() {
        return (systemTime != 0);
    }
};

#endif // DGO_VKBOT_H
