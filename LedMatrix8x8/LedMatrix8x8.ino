#define ROW_8 A0
#define ROW_7 13
#define ROW_6 12
#define ROW_5 11
#define ROW_4 10
#define ROW_3 9
#define ROW_2 8
#define ROW_1 A1

#define COL_8 A4
#define COL_7 7
#define COL_6 6
#define COL_5 5
#define COL_4 4
#define COL_3 3
#define COL_2 2
#define COL_1 A5


const int rowPins[] = {ROW_1, ROW_2, ROW_3, ROW_4, ROW_5, ROW_6, ROW_7, ROW_8};

const int interval = 1;

const byte image[] = {
  // Сердечко
  B10011001,
  B01100110,
  B01111110,
  B01111110,
  B01111110,
  B10111101,
  B11011011,
  B11100111

};

void row_ignition(byte state, int row) {
  digitalWrite(COL_8, (state >> 0) & 1);
  digitalWrite(COL_7, (state >> 1) & 1);
  digitalWrite(COL_6, (state >> 2) & 1);
  digitalWrite(COL_5, (state >> 3) & 1);
  digitalWrite(COL_4, (state >> 4) & 1);
  digitalWrite(COL_3, (state >> 5) & 1);
  digitalWrite(COL_2, (state >> 6) & 1);
  digitalWrite(COL_1, (state >> 7) & 1);
  digitalWrite(row, HIGH);

}

void row_off(int row);

void setup() {
  Serial.begin(9600);
  pinMode(ROW_7, OUTPUT);
  pinMode(ROW_6, OUTPUT);
  pinMode(ROW_5, OUTPUT);
  pinMode(ROW_4, OUTPUT);
  pinMode(ROW_3, OUTPUT);
  pinMode(ROW_2, OUTPUT);
  pinMode(ROW_1, OUTPUT);
  pinMode(ROW_8, OUTPUT);

  pinMode(COL_7, OUTPUT);
  pinMode(COL_6, OUTPUT);
  pinMode(COL_5, OUTPUT);
  pinMode(COL_4, OUTPUT);
  pinMode(COL_3, OUTPUT);
  pinMode(COL_2, OUTPUT);
  pinMode(COL_1, OUTPUT);
  pinMode(COL_8, OUTPUT);

}

unsigned long start_time = millis();

void loop() {

  for (int i = 0; i < 8; i++) {
  
    row_ignition(image[i], rowPins[i]);
    while(true) {
      unsigned long cur_time = millis();
      if (cur_time - start_time >= interval) {
        start_time = cur_time;
        break;
      }
    }

    row_off(rowPins[i]);
  }
  
}

void row_off(int row) {
  digitalWrite(row, LOW);

  digitalWrite(COL_7, LOW);
  digitalWrite(COL_6, LOW);
  digitalWrite(COL_5, LOW);
  digitalWrite(COL_4, LOW);
  digitalWrite(COL_3, LOW);
  digitalWrite(COL_2, LOW);
  digitalWrite(COL_1, LOW);
  digitalWrite(COL_8, LOW);
}
