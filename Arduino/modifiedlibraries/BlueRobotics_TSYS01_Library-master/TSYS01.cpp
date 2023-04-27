#include "TSYS01.h"
#include <Wire.h>

#define TSYS01_ADDR                        0x77  
#define TSYS01_RESET                       0x1E
#define TSYS01_ADC_READ                    0x00
#define TSYS01_ADC_TEMP_CONV               0x48
#define TSYS01_PROM_READ                   0XA0

TSYS01::TSYS01() {

}

bool TSYS01::init() {
	// Reset the TSYS01, per datasheet
	Wire.beginTransmission(TSYS01_ADDR);
	Wire.write(TSYS01_RESET);
	Wire.endTransmission();
	
	delay(10);
	
		// Read calibration values
	for ( uint8_t i = 0 ; i < 8 ; i++ ) {
		Wire.beginTransmission(TSYS01_ADDR);
		Wire.write(TSYS01_PROM_READ+i*2);
		Wire.endTransmission();

		Wire.requestFrom(TSYS01_ADDR,2);
		C[i] = (Wire.read() << 8) | Wire.read();
	}

	// Verify that data is correct with CRC
	uint8_t crcRead = C[0] >> 12;
	uint8_t crcCalculated = crc4(C);

	if ( crcCalculated == crcRead ) {
		return true; // Initialization success
	}

	return false; // CRC fail

}

void TSYS01::read() {
	
	Wire.beginTransmission(TSYS01_ADDR);
	Wire.write(TSYS01_ADC_TEMP_CONV);
	Wire.endTransmission();
 
	delay(10); // Max conversion time per datasheet
	
	Wire.beginTransmission(TSYS01_ADDR);
	Wire.write(TSYS01_ADC_READ);
	Wire.endTransmission();

	Wire.requestFrom(TSYS01_ADDR,3);
	D1 = 0;
	D1 = Wire.read();
	D1 = (D1 << 8) | Wire.read();
	D1 = (D1 << 8) | Wire.read();

	calculate();
}

void TSYS01::readTestCase() {
	C[0] = 0;
	C[1] = 28446;  //0xA2 K4
	C[2] = 24926;  //0XA4 k3
 	C[3] = 36016;  //0XA6 K2
	C[4] = 32791;  //0XA8 K1
	C[5] = 40781;  //0XAA K0
	C[6] = 0;
	C[7] = 0;

	D1 = 9378708.0f;
	
	adc = D1/265; 

	calculate();
}

void TSYS01::calculate() {	
	adc = D1/256; 

TEMP = ((-2) * float(C[1]) / 1000000000000000000000.0f * pow(adc,4) + 
        4 * float(C[2]) / 10000000000000000.0f * pow(adc,3) +
	  (-2) * float(C[3]) / 100000000000.0f * pow(adc,2) +
   	    1 * float(C[4]) / 1000000.0f * adc +
      (-1.5) * float(C[5])  / 100)  ;


}

float TSYS01::temperature() {
	return TEMP;
}

uint8_t TSYS01::crc4(uint16_t n_prom[]) {
	uint16_t n_rem = 0;

	n_prom[0] = ((n_prom[0]) & 0x0FFF);
	n_prom[8] = 0;

	for ( uint8_t i = 0 ; i < 16; i++ ) {
		if ( i%2 == 1 ) {
			n_rem ^= (uint16_t)((n_prom[i>>1]) & 0x00FF);
		} else {
			n_rem ^= (uint16_t)(n_prom[i>>1] >> 8);
		}
		for ( uint8_t n_bit = 8 ; n_bit > 0 ; n_bit-- ) {
			if ( n_rem & 0x8000 ) {
				n_rem = (n_rem << 1) ^ 0x3000;
			} else {
				n_rem = (n_rem << 1);
			}
		}
	}
	
	n_rem = ((n_rem >> 12) & 0x000F);

	return n_rem ^ 0x00;
}
