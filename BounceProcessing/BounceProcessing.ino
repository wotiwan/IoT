#define BUTTON_PIN 2 
#define BOUNCE_TIME 50
#define PRESSED HIGH
volatile long int cur_time = 0;
volatile long int press_time = 0;
volatile bool pressed_candidate = false;
volatile long int hold_time = 0;
volatile int press_count = 0;
volatile bool report_to_user = false;
bool button_pressed = false;

void setup() {
  pinMode(BUTTON_PIN, INPUT);
  Serial.begin(9600);
  attachInterrupt(0, process_button_click, RISING);
}

void loop() {
  if(report_to_user == true) {
      Serial.println("Press candidate");
    report_to_user = false;
  }

  if (!button_pressed) {
    press_validation();
  }
  
  if (digitalRead(BUTTON_PIN) != PRESSED) {
    pressed_candidate = false;
    if (button_pressed) {
      button_pressed = false;
      hold_time = millis() - press_time;
      Serial.print("hold time is:");
      Serial.println(hold_time);
    }
  }

}

void press_validation() {
  cur_time = millis();
  if (press_time + BOUNCE_TIME == cur_time && digitalRead(BUTTON_PIN) == PRESSED) {
    ++press_count;
    Serial.print("Press count is:");
    Serial.println(press_count);
    button_pressed = true;
  } else {
    pressed_candidate = false;
  }
}

void process_button_click() {
  if (pressed_candidate == false) {
    press_time = millis();
    pressed_candidate = true;
    report_to_user = true;
    hold_time = 0;
  }
}
