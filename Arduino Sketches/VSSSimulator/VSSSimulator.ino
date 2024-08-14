const int powerPin1 = 10;
const int powerPin2 = 11;
float frequency = 50.0f;
float microsecondsDelay = 100.0f;//1000000.0f / (1020.0f * frequency);

float lastTime = 0.0f;
int countedHZ = 0;
int intervall = 0;

void setup() {
  pinMode(powerPin1, OUTPUT);
  pinMode(powerPin2, OUTPUT);
  Serial.begin(9600);
  Serial.println("Beginning");
  Serial.println(microsecondsDelay);
}

int last_pin_state = LOW;
int count = 0;
float time = 0.0f;

void loop() {
  /*for(int i = 0; i < 255; i++) {
    analogWrite(powerPin1, i);
    delayMicroseconds(microsecondsDelay);
  }
  for(int i = 255; i >= 0; i--) {
    analogWrite(powerPin1, i);
    delayMicroseconds(microsecondsDelay);
  }*/

  //for(int i = 0; i < 255; i++) {
    analogWrite(powerPin2, HIGH);
    delayMicroseconds(microsecondsDelay);
  //}
  //for(int i = 255; i >= 0; i--) {
    analogWrite(powerPin2, LOW);
    delayMicroseconds(microsecondsDelay);
  //}

  countedHZ++;
  if(millis() - lastTime >= 1000.0f) {
    Serial.print(countedHZ);
    Serial.print(" - ");
    Serial.println(microsecondsDelay);

    lastTime = millis();
    countedHZ = 0;
  }


}