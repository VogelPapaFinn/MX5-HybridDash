// The values which will be updated every loop iteration
float measuredRPMCycle = 0;
float measuredSpeedCycle = 0;
float measuredFuelLevel = 0;
float measuredTemperature = 0;
float measuredOilPressure = 0;

void setup() {
  // Initialize Serial
  Serial.begin(115200);
}

void loop() {
  // Send the new data to the RaspberryPi
  Serial.print("DATA;");

  Serial.print("RPM="); 
  Serial.print(measuredRPMCycle); 
  Serial.print(";");

  Serial.print("SPEED=");
  Serial.print(measuredSpeedCycle);
  Serial.print(";");

  Serial.print("FUEL=");
  Serial.print(measuredFuelLevel);
  Serial.print(";");

  Serial.print("TEMP=");
  Serial.print(measuredTemperature);
  Serial.print(";");

  Serial.print("OILP=");
  Serial.print(measuredOilPressure);
  Serial.print(";");

  Serial.print("END;");

  // DEBUG DELAY, NEEDS TO BE REMOVED ON FINAL VERSION
  delay(1000);
}
