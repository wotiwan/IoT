#define SENSOR_IN_0 A0
#define SENSOR_IN_1 A1

#define DIOD_1 13
#define DIOD_2 12

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(DIOD_1, OUTPUT);
  pinMode(DIOD_2, OUTPUT);
  digitalWrite(DIOD_1, HIGH);
  digitalWrite(DIOD_2, HIGH);
}
long previous_millis = 0;
long interval = 1000;
long interval_2 = 500;
bool is_equal = false;

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();
  if (currentMillis - previous_millis == interval_2 && !is_equal) {
    digitalWrite(DIOD_1, LOW);
    digitalWrite(DIOD_2, LOW);
  }
  if (currentMillis - previous_millis == interval) {
    previous_millis = currentMillis;
    int photo_val_0 = analogRead(SENSOR_IN_0);
    int photo_val_1 = analogRead(SENSOR_IN_1);
    Serial.print("Photo val A1 = ");
    Serial.print(photo_val_0);
    Serial.print("\n");
    
    Serial.print("Photo val A2 = ");
    Serial.print(photo_val_1);
    Serial.print("\n");

    if (photo_val_0 - photo_val_1 > 5) {
      is_equal = false;
      digitalWrite(DIOD_1, HIGH);
      digitalWrite(DIOD_2, LOW);
    } else if (photo_val_1 - photo_val_0 > 5 ) {
      is_equal = false;
      digitalWrite(DIOD_1, LOW);
      digitalWrite(DIOD_2, HIGH);
    } else {
      is_equal = true;
      digitalWrite(DIOD_1, HIGH);
      digitalWrite(DIOD_2, HIGH);
    }
  }

}