#define LED_PIN_GREEN 11
#define LED_PIN_YELLOW 10
#define LED_PIN_RED 9

#define SET_AUTO 'a'
#define SET_RED 'r'
#define SET_GREEN 'g'
#define SET_MANUAL 'm'

#define STATE_AUTO 1
#define M_STATE_RED 2
#define M_STATE_GREEN 3
#define STATE_MANUAL 4


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(LED_PIN_YELLOW, OUTPUT);
  pinMode(LED_PIN_RED, OUTPUT);
}

long green_interval = 10000;
long blinking_green_interval = 500;
long yellow_interval = 1000;
long red_interval = 7000;

void auto_change() {
  unsigned long start_time = millis();
  while(true) {
    unsigned long current_time = millis();
    digitalWrite(LED_PIN_GREEN, HIGH);
    if (current_time - start_time == green_interval) {
      digitalWrite(LED_PIN_GREEN, LOW);
      start_time = current_time;
      break;
    }
  }
  int step = 1;
  while(true) {
    unsigned long current_time = millis();
    if (current_time - start_time == blinking_green_interval && step == 1) {
      digitalWrite(LED_PIN_GREEN, HIGH);
      start_time = current_time;
      step++;
    }
    if (current_time - start_time == blinking_green_interval && step == 2) {
      digitalWrite(LED_PIN_GREEN, LOW);
      start_time = current_time;
      step++;
    }
    if (current_time - start_time == blinking_green_interval && step == 3) {
      digitalWrite(LED_PIN_GREEN, HIGH);
      start_time = current_time;
      step++;
    }
    if (current_time - start_time == blinking_green_interval && step == 4) {
      digitalWrite(LED_PIN_GREEN, LOW);
      digitalWrite(LED_PIN_YELLOW, HIGH);
      start_time = current_time;
      break;
    }
  }

  while(true) {
    unsigned long current_time = millis();
    if (current_time - start_time == yellow_interval) {
      digitalWrite(LED_PIN_YELLOW, LOW);
      digitalWrite(LED_PIN_RED, HIGH);
      start_time = current_time;
      break;
    }
  }

  while(true) {
    unsigned long current_time = millis();
    if (current_time - start_time == red_interval) {
      digitalWrite(LED_PIN_RED, LOW);
      digitalWrite(LED_PIN_YELLOW, HIGH);
      start_time = current_time;
      break;
    }
  }

  while(true) {
    unsigned long current_time = millis();
    if (current_time - start_time == yellow_interval) {
      digitalWrite(LED_PIN_YELLOW, LOW);
      start_time = current_time;
      break;
    }
  }
}

int state = -1;
int M_state = 4;
unsigned long manual_timer = 0;
bool manual_timer_started = false;

void update_state(){
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if(cmd == SET_AUTO) {
      state = STATE_AUTO;
    } else if (cmd == SET_MANUAL) {
      manual_timer = millis();
      manual_timer_started = true;
      state = STATE_MANUAL;
      M_state = STATE_MANUAL;
      Serial.println("You are now in manual");
    } else if (cmd == SET_RED && state == STATE_MANUAL) {
      if (M_state == M_STATE_GREEN) {
        change_through_yellow();
      }
      M_state = M_STATE_RED;
    } else if (cmd == SET_GREEN && state == STATE_MANUAL) {
      if (M_state == M_STATE_RED) {
        change_through_yellow();
      }
      M_state = M_STATE_GREEN;
    } else {
      Serial.println("Unknown command");
    }
  }
  if (manual_timer_started) {
    unsigned long current_time = millis();
    if (current_time - manual_timer == 60000) {
      state = STATE_AUTO;
      Serial.println("Coming back to auto.");
      manual_timer_started = false;
    }
  }
}

void leds_off() {
  digitalWrite(LED_PIN_GREEN, LOW);
  digitalWrite(LED_PIN_YELLOW, LOW);
  digitalWrite(LED_PIN_RED, LOW);
}

void change_through_yellow() {
  unsigned long start_time = millis();
  leds_off();
  digitalWrite(LED_PIN_YELLOW, HIGH);
  while(true) {
    unsigned long current_time = millis();
    if (current_time - start_time == yellow_interval) {
      digitalWrite(LED_PIN_YELLOW, LOW);
      break;
    }
  }
}

void process_state(){
  if (state == STATE_AUTO){
    leds_off();
    auto_change();
  } else if (state == STATE_MANUAL && M_state == STATE_MANUAL) {
    leds_off();
    digitalWrite(LED_PIN_YELLOW, HIGH);
  } else if (state == STATE_MANUAL && M_state == M_STATE_GREEN) {
    leds_off();
    digitalWrite(LED_PIN_GREEN, HIGH);
  } else if (state == STATE_MANUAL && M_state == M_STATE_RED) {
    leds_off();
    digitalWrite(LED_PIN_RED, HIGH);
  }
}

void loop() {
  update_state();
  process_state();
}