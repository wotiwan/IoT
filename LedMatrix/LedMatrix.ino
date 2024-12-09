#define ROW_2 13
#define ROW_1 12
#define COL_2 7
#define COL_1 6

int ind = 0;
unsigned long start_time = millis();
long impulse_width = 10;
long pos_time_start = millis();
long each_pos_time = 3000;

const byte states_matrix[] = {
  B10010000, // 1
  B10100000, // 2
  B01010000, // 3
  B01100000, // 4
  B10000000, // 1, 2
  B01000000, // 3, 4
  B11010000, // 1, 3
  B11100000, // 2, 4
  // Далее идут свитчи
  B10010110, // 1, 4
  B10100101, // 2, 3
  B10000101, // 1, 2, 3
  B10100100, // 2, 3, 4
  B10010100, // 3, 4, 1
  B01101000, // 4, 1, 2
  B10000100, // 1, 2, 3, 4
  B00000000 // all off
};

void DWM(byte state) {
  unsigned long current_time = millis();
  int bit = (state >> 0) & 1;
  digitalWrite(COL_2, bit);
  digitalWrite(COL_1, (state >> 1) & 1);
  digitalWrite(ROW_2, (state >> 2) & 1);
  digitalWrite(ROW_1, (state >> 3) & 1);

  while (current_time - start_time < impulse_width / 2) {
    current_time = millis();
  };

  digitalWrite(COL_2, (state >> 4) & 1);
  digitalWrite(COL_1, (state >> 5) & 1);
  digitalWrite(ROW_2, (state >> 6) & 1);
  digitalWrite(ROW_1, (state >> 7) & 1);

  while (current_time - start_time < impulse_width) {
    current_time = millis();
  };
  start_time = millis();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(ROW_2, OUTPUT);
  pinMode(ROW_1, OUTPUT);
  pinMode(COL_2, OUTPUT);
  pinMode(COL_1, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long current_time = millis();
  if (current_time - pos_time_start >= each_pos_time) {
    ind++;
    ind = ind % 16;
    pos_time_start = current_time;
  }
  DWM(states_matrix[ind]);
}
