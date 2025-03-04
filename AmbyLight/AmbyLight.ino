#include <FastLED.h>

#define LED_PIN 8
#define LED_COUNT 5

CRGB leds[LED_COUNT]; // Объявление массива светодиодов 

int mode = 0; // Изначальный режим не должен ждать никаких значений.

int brightness = 100; // Яркость, задаётся пользователем, по умолчанию максимальная
int effects_speed = 10; // Скорость световых эффектов, шкала от 1 до 10, задаётся пользователем

int global_hsv = 0; // мб заменить на флоат? если хсв не целочисленный
int hsv_states[LED_COUNT] = {0};
bool rainbow_set = false;
bool gradient_set = false;

void off_the_lights();
void ambilight();
void static_lights();
void set_rainbow();
void rainbow();
void set_gradient();
void gradient();

void custom_delay();

void setup() {
  Serial.begin(250000);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT);
}

void loop() {
  if (Serial.available() > 0) {
    mode = Serial.read();
    Serial.println(mode);
    if (mode != 3) {
      rainbow_set = false;
      global_hsv = 0;
    }
    if (mode != 4) {
      gradient_set = false;
      global_hsv = 0; // потом поломается это, надо по хорошему разные переменные хсв для градиента и радуги
    }
  }

  if (mode == 0) { // может вызвать проблемы | выключение подсветки
    off_the_lights();
  }

  if (mode == 1) { // Адаптивная эмбиент подсветка
    ambilight();
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
}

void off_the_lights() {
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = 0;
  }
  FastLED.show();
  Serial.println("OK");
}

void static_lights() {
  while(Serial.available() < 3) {
    // Ждем весь пакет опять же
  }

  leds[0].b = Serial.read();  // Читаем R
  leds[0].g = Serial.read();  // Читаем G
  leds[0].r = Serial.read();  // Читаем B
  for (int i = 1; i < LED_COUNT; i++) {
    leds[i].b = leds[0].b; // Копируем полученный цвет во все диоды  
    leds[i].g = leds[0].g;
    leds[i].r = leds[0].r;
  }
  FastLED.show();
  Serial.println("OK");
}

void ambilight() {
  while(Serial.available() < LED_COUNT * 3) {
    // просто ждём пока придут все данные
  } 
  Serial.read(); // От куда то берётся 1 байт мусора 
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i].b = Serial.read();  // Читаем R
    leds[i].g = Serial.read();  // Читаем G
    leds[i].r = Serial.read();  // Читаем B
    if (leds[i].r <= 20 && leds[i].g <= 20 && leds[i].b <= 20) { // Небольшой трешхолд для серых цветов
      leds[i].r = 0;
      leds[i].g = 0;
      leds[i].b = 0;
    }
  }
  FastLED.show();

  while(Serial.available()) {
    Serial.read();
  }

  Serial.println("OK"); // питон ждёт пока мы закончим прежде чем отправить новый пакет
}

void set_rainbow() { // Радуга // Проверено на питоне, цвета идеально правильно вычисляются, кольцо идеальное
  // Проходим по хсв 0..255 два раза за 228 диодов. 510 цветов, шаг между диодами 2 цвета. + каждый 4 диод шаг 3
  // 54 диода с шагом 3 = 162 цвета, 174 диода с шагом 2 = 348 цветов, итого 510 цветов
  for (int i = 0; i < LED_COUNT; i++) { // установка стартового состояния
    if (global_hsv >= 255) { // Если дошли до предела - перешли на следующий круг
      global_hsv = global_hsv % 255;
    }
    leds[i].setHSV(global_hsv, 255, brightness); 
    hsv_states[i] = global_hsv;

    // Для тестов, на полной ленте раскомментировать
    global_hsv += 35; // А это удалить
    // if (i % 4 == 0) { 
    //   global_hsv += 3;
    // } else {
    //   global_hsv += 2;
    // }
  }
  FastLED.show();
  rainbow_set = true;

}

void rainbow() {
  if (!rainbow_set) {
    set_rainbow(); // в лупе делать нельзя, будет постоянно в стартовой позиции
  }

  for (int i = 0; i < LED_COUNT; i++) {
    int new_color = (hsv_states[i] + 1) % 256;
    hsv_states[i] = new_color;
    leds[i].setHSV(new_color, 255, brightness); // меняем оттенок на 1 (получится змейка же D: )
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

void gradient() { // Градиент не управляемый по идее

  if (!gradient_set) { // Проверка стартового состояния
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

void custom_delay() {
  long int start_time = millis();
  long int cur_time = millis();
  int delay_ratio = 11 - effects_speed; // 
  int delay = 15 * delay_ratio;
  while (true) {
    if (cur_time - delay >= start_time) {
      return;
    }
    cur_time = millis();
  }
}
