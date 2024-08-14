int IRQcount;
int pin = 3;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(pin, INPUT);
  Serial.print("Beginning");
  attachInterrupt(digitalPinToInterrupt(pin), IRQcounter, RISING);
}

void IRQcounter() {
  IRQcount++;
}

void loop() {
  // put your main code here, to run repeatedly:
  int result = IRQcount;
  IRQcount = 0;
  Serial.print("Counted = ");
  Serial.println(result);
  delay(1000);
}