#include <FastLED.h>

// #define LED_PIN 8
#define LED_PIN D1
#define LED_COUNT 228

CRGB leds[LED_COUNT]; // Объявление массива светодиодов

int mode = 3; // Изначальный режим не должен ждать никаких значений.

int saturation = 255; // Насыщенность HSV цвета, задаётся пользователем ?? Если колорпикер будет хсвшным, что вряд ли
int brightness = 85; // Яркость, задаётся пользователем, по умолчанию (!максимальная)
int effects_speed = 10; // Скорость световых эффектов, шкала от 1 до 10, задаётся пользователем

int global_rgb[3] = {255};
int global_hsv = 0;
int hsv_states[LED_COUNT] = {0};
bool rainbow_set = false;
bool gradient_set = false;
bool star_set = false;
int ambilight_iteration = 1;

int star_position = 73; // стартовая позиция

void off_the_lights();
void ambilight(int iteration);
void static_lights();
void set_rainbow();
void rainbow();
void set_gradient();
void gradient();
void set_star();
void star_shooting();

void change_brightness(int old_mode);
void change_rgb_color();

void custom_delay();

void setup() {
  Serial.begin(921600);
  Serial.setRxBufferSize(1024);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT);
}

void loop() {
  if (Serial.available() > 0) {

    int new_mode = Serial.read();
    

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

  Serial.println(mode);

  if (mode == 0) { // может вызвать проблемы | выключение подсветки
    off_the_lights();
  }

  if (mode == 1) { // Адаптивная эмбиент подсветка
    ambilight_iteration = 1; // Всего 11 + 1 = 12 пакетов
    ambilight(ambilight_iteration);
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

void ambilight(int iteration) {
  
  // int bite_quantity = 60;
  // int start_position = (iteration - 1) * bite_quantity / 3; 


  if (iteration == 1) {
    Serial.println("start");
  }

  if (iteration != 12) {
    // ждём пока придёт 684 байта 
    while(Serial.available() < 684) { // Exception(4) + soft reload если данные долго не приходят
      delay(1); // Чтобы не улетать в reload, поменять потом на кастом
    } 
  }
  if (iteration == 1) {
    Serial.read();
  }

  for (int i = 0; i < LED_COUNT; i++) {
    leds[i].r = Serial.read();  // Читаем R
    leds[i].g = Serial.read();  // Читаем G
    leds[i].b = Serial.read();  // Читаем B
    if (leds[i].r <= 20 && leds[i].g <= 20 && leds[i].b <= 20) { // Небольшой трешхолд для серых цветов
      leds[i].r = 0;
      leds[i].g = 0;
      leds[i].b = 0;
    }
  }

  while(Serial.available()) { 
    Serial.read();
  }

  if (iteration != 1) {
    Serial.println("next");
    ambilight(iteration + 1);
  } else {
    FastLED.show();
    Serial.println("OK_ambi");
  }
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
    // global_hsv += 35; // А это удалить
    if (i % 4 == 0) {
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

void set_star() {
  while(Serial.available() < 3) {
    // Ждем пакет
  }
  leds[star_position].r = Serial.read();
  leds[star_position].g = Serial.read();
  leds[star_position].b = Serial.read();
  star_set = true;
}

void star_shooting() {

  if (!star_set) {
    set_star();
  }

  int star_brightness = brightness;
  for (int i = star_position; i > 72; i-- ) {
    
    int brightness_ratio = star_position - i * 10; // степень затухания

    leds[i].r = max(leds[star_position].r - brightness_ratio, 0);
    leds[i].g = max(leds[star_position].g - brightness_ratio, 0);
    leds[i].b = max(leds[star_position].b - brightness_ratio, 0);
  }
  star_position = min(star_position+1, 214); // Она будет упираться в низ экрана и исчезать, а не уезжать за него.
  FastLED.show();
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

void change_brightness() {
  while (Serial.available() < 1) {
    // Ожидаем пакет
  }
  brightness = Serial.read();
  while(Serial.available()) {
    Serial.read();
  }
  Serial.println("OK_brightness");
}

void change_rgb_color() {
  Serial.println("start_rgb");
  while (Serial.available() < 3) {
    delay(1); // Ожидаем пакет // Сменить на кастом
  }
  global_rgb[0] = Serial.read();
  global_rgb[1] = Serial.read();
  global_rgb[2] = Serial.read();
  while(Serial.available()) {
    Serial.read();
  }
  Serial.println("OK_rgb");
}
