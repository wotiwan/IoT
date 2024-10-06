#define DIOD 11

#define LED_ON 'u'
#define LED_OFF 'd'

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(DIOD, OUTPUT);
}

void update_state(){
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if(cmd == LED_ON) {
      digitalWrite(DIOD, HIGH);
    } else if(cmd == LED_OFF) {
      digitalWrite(DIOD, LOW);
    } else {
      Serial.println("Unknown command");
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  update_state();
}
