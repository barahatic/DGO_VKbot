// Пример: Эхо-бот
// Отвечает на все сообщения эхом

#include <DGO_VKbot.h>

// НАСТРОЙКИ
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
#define VK_TOKEN "your_vk_token_here"
#define GROUP_ID "-your_group_id"  // ID группы с минусом!

// Создаем экземпляр бота
DGO_VKbot bot;

// Обработчик новых сообщений
void onNewMessage(VkUpdate& update) {
  if (update.type == VK_MESSAGE_NEW) {
    String text = update.message.text;
    int peer_id = update.message.peer_id;
    
    Serial.print("Получено сообщение от ");
    Serial.print(update.message.from_id);
    Serial.print(": ");
    Serial.println(text);
    
    // Отправляем эхо-ответ
    String reply = "Эхо: " + text;
    bot.sendMessage(reply, peer_id);
    
    Serial.println("Ответ отправлен");
  }
}

void setup() {
  Serial.begin(115200);
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
    bot.setTimezone(3); // UTC+3 для Москвы
    bot.syncTime();
  } else {
    Serial.println("Ошибка запуска бота!");
  }
}

void loop() {
  // Обрабатываем события
  bot.tick();
  delay(100);
}
