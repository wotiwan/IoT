#define DATA_PIN 4 
#define CLOCK_PIN 2 
#define LATCH_PIN 3

// 8 1 2 5 4 3 6 7
bool digits[10][8] = {
  {1,0,0,0,0,0,0,1},  // 0
  {1,1,0,1,1,0,1,1},  // 1
  {1,0,0,0,0,1,1,0},  // 2
  {1,0,0,1,0,0,1,0},  // 3
  {1,1,0,1,1,0,0,0},  // 4
  {1,0,1,1,0,0,0,0},  // 5
  {1,0,1,0,0,0,0,0},  // 6
  {1,0,0,1,1,0,1,1},  // 7
  {1,0,0,0,0,0,0,0},  // 8
  {1,0,0,1,0,0,0,0}   // 9
};

int recieved_value = 99;
bool new_value_recieved = false;
int cur_seconds = recieved_value;
int first_num = cur_seconds / 10;
int second_num = cur_seconds % 10;
int cur_pos = 0;
int timer = 0;

void setup() 
{
  DDRD = B00011100;
  PORTD = B00000000;
  Serial.begin(9600);
  TCCR2A = 0;            
  TCCR2B = (1 << CS22); 
  TIMSK2 = (1 << TOIE2);
}

ISR(TIMER2_OVF_vect) {
  ++timer;
  if (timer == 62) { // срабатывание каждые 992 миллисекунды
    timer = 0;
    if (cur_pos < 8) {
      bool state = digits[second_num][7-cur_pos];
      data_pin_write(state);
      cur_pos++;
    } else {
      bool state = digits[first_num][15-cur_pos];
      data_pin_write(state);
      cur_pos++;
    } 
    
    PORTD |= (1 << CLOCK_PIN);
    PORTD &= ~(1 << CLOCK_PIN);

    if (cur_pos == 16) {
      cur_pos = 0;

      if (cur_seconds != 0) {
        cur_seconds--;
      } else {
        cur_seconds = recieved_value;
      }
      first_num = cur_seconds / 10;
      second_num = cur_seconds % 10;
      PORTD |= (1 << LATCH_PIN);
      PORTD &= ~(1 << LATCH_PIN);
      check_updates();
    }
  }
}

void check_updates() {
  if (new_value_recieved) {
      cur_seconds = recieved_value;
      first_num = cur_seconds / 10;
      second_num = cur_seconds % 10;
      cur_pos = 0;
      new_value_recieved = false;
  }
}

void data_pin_write(bool state) {
  if (state) {
    PORTD |= (1 << DATA_PIN);
  } else {
    PORTD &= ~(1 << DATA_PIN);
  }
}

void loop() 
{
  if (Serial.available() > 0) {  
    int input = Serial.parseInt(); 
    if (input >= 0 && input < 100) {  
      new_value_recieved = true;
      recieved_value = input;  
    }
  }
}
