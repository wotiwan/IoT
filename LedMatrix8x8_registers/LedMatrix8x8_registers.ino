void setup() {
  cli(); // stop interrupts
  DDRC = B00110011; //
  DDRB = B00111111; // Устанавливаем pinmode (output/input (1/0))
  DDRD = B11111100; //

  PORTD = B00000000; // Все пины порта D в LOW

  TCCR2A = 0;
  TCCR2B = 0;
  TCCR2B = TCCR2B | (1 << CS11);
  TIMSK2 = TIMSK2 | (1 << TOIE2); 
  sei(); // allow interrupts
}

const byte images[][8] = {
  {
    B10000001,
    B00000000,
    B00111100,
    B00111111,
    B10000111,
    B11100111,
    B11111111,
    B11100111
  },
  {
    // Сердечко
    B10011001,
    B01100110,
    B01111110,
    B01111110,
    B01111110,
    B10111101,
    B11011011,
    B11100111
  }
};

int cur_row = 0;
int frames_counter = 0;
int cur_img = 0;


ISR(TIMER2_OVF_vect) {
  row_off();

  if (frames_counter == 1000) {
    cur_img = 0;
  } else if (frames_counter == 2000) {
    cur_img = 1;
    frames_counter = 0;
  }

  row_ignition(cur_row, images[cur_img][cur_row]);
  if (cur_row < 7) {
    cur_row++;
  }
  else {
    cur_row = 0;
    frames_counter++;
  }
}

void row_ignition(int row, byte state) {
  // Зажигаем колонки
  customWrite(PORTC, 5, (state >> 0) & 1);
  customWrite(PORTD, 2, (state >> 1) & 1);
  customWrite(PORTD, 3, (state >> 2) & 1);
  customWrite(PORTD, 4, (state >> 3) & 1);
  customWrite(PORTD, 5, (state >> 4) & 1);
  customWrite(PORTD, 6, (state >> 5) & 1);
  customWrite(PORTD, 7, (state >> 6) & 1);
  customWrite(PORTC, 4, (state >> 7) & 1);

  if (row < 7 and row > 0 ) { // Зажигаем строку
    customWrite(PORTB, row-1, 1);
  } else if (row == 7) {
    customWrite(PORTC, 0, 1);
  } else {
    customWrite(PORTC, 1, 1);
  }
}

void customWrite(volatile uint8_t &port, uint8_t pin, int state) {
  if (state == 1) {
    port |= (1 << pin);
  }
  else {
    port &= ~(1 << pin);
  }
}

void row_off() {
  PORTD = B00000000;
  PORTB = B00000000;
  PORTC = B00000000;
}

void loop() {
}