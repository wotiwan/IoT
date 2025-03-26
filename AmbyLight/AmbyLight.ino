#include <FastLED.h>

#define LED_PIN D1
#define LED_COUNT 228

CRGB leds[LED_COUNT]; // Объявление массива светодиодов

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

    if (mode == 1) { // Адаптивная эмбиент подсветка
      ambilight();
    }
  }

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

void ambilight() {

  // ждём пока придёт 684 байта 
  while(Serial.available() < 684) { // Exception(4) + soft reload если данные долго не приходят
    delay(1); // Чтобы не улетать в reload, поменять потом на кастом
  } 

  for (int i = 0; i < LED_COUNT; i++) { 

    int new_r = Serial.read(); //
    int new_g = Serial.read(); //
    int new_b = Serial.read(); //

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
        new_r = max(new_r - 7, 0); 
        new_g = max(new_g - 7, 0);
        new_b = max(new_b - 7, 0);
        color_ratio = (255 - brightest_color - 10) / 255 * 1.35; // Коэф. также подстраивается
      }
      else if (brightest_color > 30) {
        new_r = max(new_r - 15, 0); 
        new_g = max(new_g - 15, 0);
        new_b = max(new_b - 15, 0);
        color_ratio = (255 - brightest_color) / 255; // Увеличение яркости тусклых пикселей
      }

      new_r = new_r * (color_ratio + 1) * red_ratio;
      new_g = new_g * (color_ratio + 1) * green_ratio;
      new_b = new_b * (color_ratio + 1) * blue_ratio;
    }

    // Устраняем мерцание сглаживанием
    leds[i].r = leds[i].r * (1 - smooth_ratio) + new_r * smooth_ratio;  // Читаем R
    leds[i].g = leds[i].g * (1 - smooth_ratio) + new_g * smooth_ratio;  // Читаем G
    leds[i].b = leds[i].b * (1 - smooth_ratio) + new_b * smooth_ratio;  // Читаем B

  }

  while(Serial.available()) { 
    Serial.read();
  }

  FastLED.show();
  Serial.println("OK_ambi");
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
