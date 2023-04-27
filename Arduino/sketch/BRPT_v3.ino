// Include Libraries
#include <Wire.h>
#include <TSYS01.h> // modified TSYS01 library
#include <MS5837.h> // modified MS5837 library

// Define pin for built in LED control
#define LEDpin 13
#define HWSERIAL Serial1 // Turn on hardware serial for TX1/RX1 on Teensy for logging to SLERJ
// Set I2C address for each sensor
MS5837 Psensor;
TSYS01 Tsensor;

String TSYS;
String TBAR02;
String PBAR02;


void setup() {
  Wire.begin();

  Serial.begin(115200); // Initialize serial monitor
  Serial1.begin(115200); // Initialize TX1/RX1
  pinMode(LEDpin, OUTPUT); // Set pinMode for LED control

  // Initialize pressure sensor
  // Returns true if the initialization was succesful
  // We can't continue with the rest of the program unless the sensor works properly
  Psensor.init();
  while (!Psensor.init()) {
    Serial.println("Psensor init FAIL!");
    Serial.print("\n\n\n");
    delay(5000);
  }
  Psensor.setModel(MS5837::MS5837_02BA);
  Psensor.setFluidDensity(1029);
  Psensor.read();
  Serial.println("Pressure sensor init SUCCESS!");
  Serial.print("P = "); Serial.print(Psensor.pressure() / 100.); Serial.println(" mbar \n");
  delay(1000);

  // Initialize temperature sensor
  // Returns true if initialization was successful
  // We can't continue with the rest of the program unless we can initialize the sensor
  Tsensor.init();
  Tsensor.read();

 while ((Tsensor.temperature() < 0) || (Tsensor.temperature() > 100)) {
    Serial.print("Tsensor init FAIL!");
    Tsensor.init();
    Tsensor.read();
    Serial.println("\n\n\n");
    delay(5000);
  }

  Serial.println("Temperature sensor init SUCCESS");
  Serial.print("T = "); Serial.print(Tsensor.temperature()); Serial.println(" C \n");
  delay(1000);

  // Blink 3 times to indicate successful initialization of both sensors
  for (int n = 0; n < 3; n++) {
    for (int r = 0; r <= 255; r++) {
      analogWrite(LEDpin, r);
      delay(1);
    }
    for (int r = 255; r >= 0 ; r--) {
      analogWrite(LEDpin, r);
      delay(1);
    }
  }
  Serial.println("TSYS01 [*C],TBar02 [*C], PBar02 [mbar]");
  Serial1.println("TSYS01 [*C],TBar02 [*C], PBar02 [mbar]");
}

void loop() {
  analogWrite(LEDpin, 255);
  Tsensor.read();
  Psensor.read();
  String TSYS = Tsensor.temperature();
  String TBAR02 = Psensor.temperature() / 100.;
  String PBAR02 = Psensor.pressure() / 100.;
  Serial.print(F("TSYS = "));
  Serial.print(TSYS);
  Serial.println(" *C");

  Serial.print(F("PBar02 = "));
  Serial.print(PBAR02);
  Serial.println(" mbar \n");



  Serial1.println(TSYS + "," + TBAR02 + "," + PBAR02);
//  delay(500);
  analogWrite(LEDpin, 0);
  delay(10);
}
