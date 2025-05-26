#include <WiFiManager.h>
#include <AsyncUDP.h>
#include <FastLED.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_PIN 22
#define LED_COUNT 228

// MQTT Setup
const char* mqtt_server = "176.215.235.238"; // 176.215.235.238 or 192.168.43.133
const int mqtt_port = 1884;
const char* mqtt_topic = "led/data";
WiFiClient espClient;
PubSubClient client(espClient);
byte savedPayload[128]; // 128 byte max, can change if there is a need
unsigned int savedPayloadLength = 0;

// FastLED setup
CRGB leds[LED_COUNT];
int mode = 3; // Изначальный режим не должен ждать никаких значений.
// Color calibration
// TODO: input by user
float red_ratio = 1.05;
float green_ratio = 0.98;
float blue_ratio = 0.96;
float smooth_ratio = 0.3; // Переменная для сглаживания картинки ambient режима
int saturation = 255; // Насыщенность HSV цвета, задаётся пользователем ?? Если колорпикер будет хсвшным, что вряд ли
int brightness = 85; // Яркость, задаётся пользователем, по умолчанию (!максимальная)
int effects_speed = 10; // Скорость световых эффектов, шкала от 1 до 10, задаётся пользователем

int global_rgb[3] = {255, 0, 0};
int global_hsv = 0;
int hsv_states[LED_COUNT] = {0};
bool rainbow_set = false;
bool gradient_set = false;
bool star_set = false;

int star_position = 73; // стартовая позиция

void off_the_lights();
void ambilight();
void static_lights();
void set_rainbow();
void rainbow();
void set_gradient();
void gradient();
void set_star();
void star_shooting();
void change_brightness();
void change_rgb_color();
void custom_delay();
int find_max(int a, int b, int c);

bool leds_busy = false;

int ambi_data[684];
// Создаём объект UDP cоединения
AsyncUDP udp;
// Определяем порт
const uint16_t port = 49152;
void parsePacket(AsyncUDPPacket packet)
{
  // Записываем адрес начала данных в памяти
  const uint8_t* msg = packet.data();
  // Записываем размер данных
  const size_t len = packet.length();

  // Если адрес данных не равен нулю и размер данных больше нуля...
  if (msg != NULL && len > 0) { // Захардкожено, переделать
    // Запускаем выполнение только если ранее был установлен mode == 1
    if (mode == 1) {
      for (int i = 0; i < len; i++) {
        ambi_data[i] = msg[i]; // msg[i] норм
      }
      // Для блокировки обработки смены режима посредством mqtt
      leds_busy = true;
      ambilight();
      leds_busy = false;
      packet.printf("OK_ambi");
    }
  }
}

void setup() {
  // Serial.setRxBufferSize(1024);
  Serial.begin(115200);
  setCpuFrequencyMhz(240);
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT);

  // Подключение через WiFiManager
  WiFiManager wm;
  wm.autoConnect("ESP32_AmbiLight");
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());

  // Mqtt Setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback_on_data);
  mqtt_reconnect();

  // UDP Setup
  if(udp.listen(port)) {
    // При получении пакета вызываем callback функцию
    udp.onPacket(parsePacket);
  }
}

void loop() {
  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();
  if (mode == 0) { // может вызвать проблемы | выключение подсветки
    off_the_lights();
  }

  if (mode == 2) { // Режим статичного цвета
    static_lights();
  }

  if (mode == 3) {
    rainbow();
  }

  if (mode == 4) {
    gradient();
  }

  if (mode == 5) {
    star_shooting();
  }
}

void ambilight() {
  int led_index = 0;
  for (int i = 0; i < 684; i+= 3, led_index++) { // Захардкожено, переделать 
    int new_r = ambi_data[i];
    int new_g = ambi_data[i+1];
    int new_b = ambi_data[i+2];

    if (new_r <= 20 && new_g <= 20 && new_b <= 20) { // Небольшой трешхолд для тусклых серых цветов
      new_r = 0;
      new_g = 0;
      new_b = 0;
    }
    else {

      float color_ratio = 0;

      int brightest_color = find_max(new_r, new_g, new_b);
      // Если тусклый цвет, то вычитаем белый компонент, затем увеличиваем яркость с большим коэфф.
      if (brightest_color < 80 and brightest_color > 30) { // Другими словами увеличиваем насыщенность цвета
        new_r = max(new_r - 7, 0); // 14 
        new_g = max(new_g - 7, 0); // 24
        new_b = max(new_b - 7, 0); // 4
        color_ratio = (255 - brightest_color - 10) / 255 * 1.35; // 1.65 // Коэф. также подстраивается
      }
      else if (brightest_color > 30) {
        new_r = max(new_r - 15, 0); 
        new_g = max(new_g - 15, 0);
        new_b = max(new_b - 15, 0);
        color_ratio = (255 - brightest_color) / 255; // Увеличение яркости тусклых пикселей
      }

      new_r = new_r * (color_ratio + 1) * red_ratio; // 37
      new_g = new_g * (color_ratio + 1) * green_ratio; // 64
      new_b = new_b * (color_ratio + 1) * blue_ratio; // 11
    }

    // Устраняем мерцание сглаживанием
    leds[led_index].r = leds[led_index].r * (1 - smooth_ratio) + new_r * smooth_ratio;  // Читаем R
    leds[led_index].g = leds[led_index].g * (1 - smooth_ratio) + new_g * smooth_ratio;  // Читаем G
    leds[led_index].b = leds[led_index].b * (1 - smooth_ratio) + new_b * smooth_ratio;  // Читаем B

  }

  // while(Serial.available()) { 
  //   Serial.read();
  // }
  FastLED.show();
  Serial.println("OK_ambi_serial");
}

void off_the_lights() {
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = 0;
  }
  FastLED.show();
  Serial.println("OK_off");
}

void static_lights() {
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i].r = global_rgb[0]; // Копируем полученный цвет во все диоды
    leds[i].g = global_rgb[1];
    leds[i].b = global_rgb[2];
  }
  FastLED.show();
}

void set_rainbow() {
  for (int i = 0; i < LED_COUNT; i++) { // установка стартового состояния
    if (global_hsv >= 255) { // Если дошли до предела - перешли на следующий круг
      global_hsv = global_hsv % 255;
    }
    leds[i].setHSV(global_hsv, 255, brightness);
    hsv_states[i] = global_hsv;

    if (i % 4 == 0) { // Для равномерного распределния цветов на 228 светодиодов
      global_hsv += 3;
    } else {
      global_hsv += 2;
    }
  }
  FastLED.show();
  rainbow_set = true;

}

void rainbow() {
  if (!rainbow_set) {
    set_rainbow();
  }

  for (int i = 0; i < LED_COUNT; i++) {
    int new_color = (hsv_states[i] + 1) % 256;
    hsv_states[i] = new_color;
    leds[i].setHSV(new_color, 255, brightness);
  }
  FastLED.show();
  custom_delay();
}

void set_gradient() {
  global_hsv = 200;
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i].setHSV(global_hsv, 255, brightness);
  }
  gradient_set = true;
}

void gradient() {

  if (!gradient_set) {
    set_gradient();
    FastLED.show();
    return;
  }

  for (int i = 0; i < LED_COUNT; i++) {
    leds[i].setHSV(global_hsv, 255, brightness);
  }

  global_hsv = (global_hsv + 1) % 256; // обнуляем если вылезли за предел

  FastLED.show();
  custom_delay();
}

void set_star() {

  off_the_lights();

  leds[star_position].r = global_rgb[0];
  leds[star_position].g = global_rgb[1];
  leds[star_position].b = global_rgb[2];
  FastLED.show();
  custom_delay();
  star_set = true;
}

void star_shooting() {

  if (!star_set) {
    set_star();
  }

  int star_brightness = brightness;
  for (int i = star_position; i > 72; i-- ) {
    
    int brightness_ratio = (star_position - i) * 20; // степень затухания
    if (i > 113) { // Если ушли за край экрана (стороны)
      leds[i] = 0;
    } 
    else {
      leds[i].r = max(global_rgb[0] - brightness_ratio, 0);
      leds[i].g = max(global_rgb[1] - brightness_ratio, 0);
      leds[i].b = max(global_rgb[2] - brightness_ratio, 0);
    }
  }
  
  star_position = min(star_position+1, 127);
  if (star_position == 127) {
    star_set = false;
    star_position = 73;
  }
  FastLED.show();
  custom_delay();
}

void custom_delay() { 
  long int start_time = millis();
  long int cur_time = millis();
  int delay_ratio = 11 - effects_speed;
  int delay = 15 * delay_ratio;
  while (true) {
    if (cur_time - delay >= start_time) {
      return;
    }
    cur_time = millis();
  }
}

void change_brightness() {
  brightness = savedPayload[0];
  while(Serial.available()) {
    Serial.read();
  }
  Serial.println("OK_brightness");
}

void change_rgb_color() {
  global_rgb[0] = savedPayload[0];
  global_rgb[1] = savedPayload[1];
  global_rgb[2] = savedPayload[2];
  Serial.println("OK_rgb");
}


int find_max(int a, int b, int c) {
  if (a > b && a > c) {
    return a;
  }
  else if (b > a && b > c) {
    return b;
  }
  else {
    return c;
  }
}

void callback_on_data(char* topic, byte* payload, unsigned int length) {
  // добавить блокировку обработки через leds_busy
  Serial.println("Что то пришло");
  int new_mode = payload[0];
  memcpy(savedPayload, payload, length);  // копируем байты
  savedPayloadLength = length;
  
  // Смена цвета, яркости, насыщенности не влечёт за собой смену режима
  if (new_mode == 99) { // Меняем яркость
    change_brightness();
  } 
  else if (new_mode == 98) { // Меняем цвет
    change_rgb_color();
  }
  else {
    mode = new_mode;
  }
  if (mode != 3) {
    rainbow_set = false;
    global_hsv = 0;
  }
  if (mode != 4) {
    gradient_set = false;
    global_hsv = 0; // потом поломается это, надо по хорошему разные переменные хсв для градиента и радуги
  }
  star_set = false;

}

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("Подключение к MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Подключено!");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Ошибка, rc=");
      Serial.print(client.state());
      Serial.println(" повтор через 5 сек");
      delay(5000);
    }
  }
}
