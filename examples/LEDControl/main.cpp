// Пример: Управление светодиодом через VK бота
// Команды: "включить" - включить LED, "выключить" - выключить LED

#include <DGO_VKbot.h>

// НАСТРОЙКИ
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
#define VK_TOKEN "your_vk_token_here"
#define GROUP_ID "-your_group_id"

#define LED_PIN 2  // Пин светодиода (встроенный LED на большинстве ESP)

// Создаем экземпляр бота
DGO_VKbot bot;

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
    if (text == "включить" || text == "вкл" || text == "on") {
      digitalWrite(LED_PIN, HIGH);
      bot.sendMessage("LED включен", peer_id);
      Serial.println("LED включен");
      
    } else if (text == "выключить" || text == "выкл" || text == "off") {
      digitalWrite(LED_PIN, LOW);
      bot.sendMessage("LED выключен", peer_id);
      Serial.println("LED выключен");
      
    } else if (text == "статус" || text == "status") {
      bool ledState = digitalRead(LED_PIN);
      String status = ledState ? "LED включен" : "LED выключен";
      bot.sendMessage(status, peer_id);
      
    } else {
      bot.sendMessage("Команды: включить, выключить, статус", peer_id);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Настройка пина LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
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
    
    // Отправляем уведомление о готовности
    delay(1000);
    // bot.sendMessage("Бот готов к управлению LED", YOUR_USER_ID);
  } else {
    Serial.println("Ошибка запуска бота!");
  }
}

void loop() {
  bot.tick();
  delay(100);
}
