// Пример: Датчик DHT11 через VK бота
// Команды: "температура" - получить температуру, "влажность" - получить влажность
// Требуется библиотека DHT sensor library

#include <DGO_VKbot.h>
#include <DHT.h>

// НАСТРОЙКИ
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
#define VK_TOKEN "your_vk_token_here"
#define GROUP_ID "-your_group_id"

// Настройки DHT11
#define DHTPIN 4        // Пин подключения DHT11
#define DHTTYPE DHT11   // Тип датчика

// Создаем объекты
DGO_VKbot bot;
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 2000; // Чтение каждые 2 секунды

// Обработчик новых сообщений
void onNewMessage(VkUpdate& update) {
  if (update.type == VK_MESSAGE_NEW) {
    String text = update.message.text;
    text.toLowerCase();
    text.trim();
    
    int peer_id = update.message.peer_id;
    
    Serial.print("Получено сообщение: ");
    Serial.println(text);
    
    // Обработка команд
    if (text == "температура" || text == "temp" || text == "t") {
      float temp = dht.readTemperature();
      if (!isnan(temp)) {
        String reply = "Температура: " + String(temp, 1) + " °C";
        bot.sendMessage(reply, peer_id);
      } else {
        bot.sendMessage("Ошибка чтения температуры", peer_id);
      }
      
    } else if (text == "влажность" || text == "humidity" || text == "h") {
      float humidity = dht.readHumidity();
      if (!isnan(humidity)) {
        String reply = "Влажность: " + String(humidity, 1) + " %";
        bot.sendMessage(reply, peer_id);
      } else {
        bot.sendMessage("Ошибка чтения влажности", peer_id);
      }
      
    } else if (text == "данные" || text == "data" || text == "d") {
      float temp = dht.readTemperature();
      float humidity = dht.readHumidity();
      
      if (!isnan(temp) && !isnan(humidity)) {
        String reply = "Температура: " + String(temp, 1) + " °C\n";
        reply += "Влажность: " + String(humidity, 1) + " %";
        bot.sendMessage(reply, peer_id);
      } else {
        bot.sendMessage("Ошибка чтения данных с датчика", peer_id);
      }
      
    } else if (text == "помощь" || text == "help") {
      String help = "Команды:\n";
      help += "температура - температура\n";
      help += "влажность - влажность\n";
      help += "данные - все данные\n";
      help += "помощь - эта справка";
      bot.sendMessage(help, peer_id);
      
    } else {
      bot.sendMessage("Используйте команду 'помощь' для списка команд", peer_id);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Инициализация DHT11
  dht.begin();
  Serial.println("DHT11 инициализирован");
  
  Serial.println("Подключение к WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi подключен!");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());
  
  // Настраиваем бота
  bot.setToken(VK_TOKEN);
  bot.setGroupId(GROUP_ID);
  bot.attach(onNewMessage);
  
  // Запускаем бота
  Serial.println("Запуск VK бота...");
  if (bot.begin()) {
    Serial.println("Бот успешно запущен!");
    
    // Синхронизируем время
    bot.setTimezone(3); // UTC+3
    bot.syncTime();
    
    // Первое чтение для стабилизации
    delay(2000);
    dht.readTemperature();
    dht.readHumidity();
    
  } else {
    Serial.println("Ошибка запуска бота!");
  }
}

void loop() {
  bot.tick();
  
  // Периодическое чтение датчика (для стабилизации)
  if (millis() - lastRead > READ_INTERVAL) {
    dht.readTemperature();
    dht.readHumidity();
    lastRead = millis();
  }
  
  delay(100);
}
