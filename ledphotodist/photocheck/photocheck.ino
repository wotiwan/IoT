#define SENSOR_IN A0

#define CHECK_RESIST 'p'

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void update_state() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == CHECK_RESIST) {
      int photo_val = analogRead(SENSOR_IN);
      Serial.write(photo_val/4);
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  update_state();
}
