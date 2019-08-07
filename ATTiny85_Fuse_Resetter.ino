#include "Arduino.h"
// AVR High-voltage Serial Programmer
// Originally created by Paul Willoughby 03/20/2010
// www.rickety.us slash 2010/03/arduino-avr-high-voltage-serial-programmer/
// Inspired by Jeff Keyzer mightyohm.com
// Serial Programming routines from ATtiny25/45/85 datasheet

// Desired fuse configuration
#define  HFUSE  0xDF   // Defaults for ATtiny25/45/85
#define  LFUSE  0x62

#define  RST     13    // Output to level shifter for !RESET from NPN to Pin 1
#define  CLKOUT   9    // Connect to Serial Clock Input (SCI) Pin 2
#define  DATAIN  10    // Connect to Serial Data Output (SDO) Pin 7
#define  INSTOUT 11    // Connect to Serial Instruction Input (SII) Pin 6
#define  DATAOUT 12    // Connect to Serial Data Input (SDI) Pin 5
#define  VCC      8    // Connect to VCC Pin 8

#define	 ALERT   A5    // Indicator pin

int targetHFUSE = HFUSE;

void setup() {
  // Initialize output pins as needed
  digitalWrite(RST, HIGH);  // Level shifter is inverting, this shuts off 12V
  digitalWrite(ALERT, HIGH);

	// Set up control lines for HV programming
	pinMode(VCC, OUTPUT);
	pinMode(RST, OUTPUT);
	pinMode(DATAOUT, OUTPUT);
	pinMode(INSTOUT, OUTPUT);
	pinMode(CLKOUT, OUTPUT);
	pinMode(DATAIN, OUTPUT);  // configured as input later when programming

	// Buzzer / beeper / LED
	pinMode(ALERT, OUTPUT);
	delay(150);
	digitalWrite(ALERT, LOW);

	// start serial port at 9600 bps:
	Serial.begin(9600);
}

void loop() {

	switch (establishContact()) {
		case 49:
			targetHFUSE = HFUSE;
			break;
		case 50:
			targetHFUSE = 0x5F;
			break;
		default:
			targetHFUSE = HFUSE;
	}

	Serial.println("Entering programming Mode\n");

	// Initialize pins to enter programming mode
	pinMode(DATAIN, OUTPUT);  //Temporary
	digitalWrite(DATAOUT, LOW);
	digitalWrite(INSTOUT, LOW);
	digitalWrite(DATAIN, LOW);
	digitalWrite(RST, HIGH); // Level shifter is inverting, so shuts off 12V

	// Enter High-voltage Serial programming mode
	digitalWrite(VCC, HIGH);  // Apply VCC to start programming process
	delayMicroseconds(20);
	digitalWrite(RST, LOW);   //Turn on 12v
	delayMicroseconds(10);
	pinMode(DATAIN, INPUT);   //Release DATAIN
	delayMicroseconds(300);

	//Programming mode
	int hFuse = readFuses();

	//Write hfuse if not already the value we want 0xDF
	//(to allow RST on pin 1)
	if (hFuse != targetHFUSE) {

		/*
		//Write lock
		delay(1000);
		Serial.println("Writing lock\n");
		shiftOut(MSBFIRST, 0x20, 0x4C);
		shiftOut(MSBFIRST, 0xFF, 0x2C);
		shiftOut(MSBFIRST, 0x00, 0x64);
		shiftOut(MSBFIRST, 0x00, 0x6C);

		//Write efuse
		delay(1000);
		Serial.println("Writing efuse\n");
		shiftOut(MSBFIRST, 0x40, 0x4C);
		shiftOut(MSBFIRST, 0xFF, 0x2C);
		shiftOut(MSBFIRST, 0x00, 0x66);
		shiftOut(MSBFIRST, 0x00, 0x6E);
		*/

		delay(1000);
		Serial.print("Writing hfuse as ");
		Serial.println(targetHFUSE, HEX);
		shiftOut(MSBFIRST, 0x40, 0x4C);

		// User selected option
		shiftOut(MSBFIRST, targetHFUSE, 0x2C);

		shiftOut(MSBFIRST, 0x00, 0x74);
		shiftOut(MSBFIRST, 0x00, 0x7C);
	}

	//Write lfuse
	delay(1000);
	Serial.println("Writing lfuse\n");
	shiftOut(MSBFIRST, 0x40, 0x4C);
	shiftOut(MSBFIRST, LFUSE, 0x2C);
	shiftOut(MSBFIRST, 0x00, 0x64);
	shiftOut(MSBFIRST, 0x00, 0x6C);

	// Confirm new state of play
	hFuse = readFuses();

	digitalWrite(CLKOUT, LOW);
	digitalWrite(VCC, LOW);
	digitalWrite(RST, HIGH);   //Turn off 12v

	// Let user know we're done
	digitalWrite(ALERT, HIGH);
	delay(50);
	digitalWrite(ALERT, LOW);
	delay(50);
	digitalWrite(ALERT, HIGH);

	Serial.println("\nProgramming complete. Press RESET to run again.");
	while (1==1){};
}

int establishContact() {
	Serial.println("Turn on the 12 volt power/\n\nYou can ENABLE the RST pin (as RST) "
			"to allow programming\nor DISABLE it to turn it into a (weak) GPIO pin.\n");

	// We must get a 1 or 2 to proceed
	int reply;

	do {
		Serial.println("Enter 1 to ENABLE the RST pin (back to normal)");
		Serial.println("Enter 2 to DISABLE the RST pin (make it a GPIO pin)");
		while (!Serial.available()) {
			// Wait for user input
		}
		reply = Serial.read();
	}
	while (!(reply == 49 || reply == 50));
	return reply;
}

int shiftOut(uint8_t bitOrder, byte val, byte val1) {
	int i;
	int inBits = 0;
	//Wait until DATAIN goes high
	while (!digitalRead(DATAIN)) {
		// Nothing to do here
	}

	//Start bit
	digitalWrite(DATAOUT, LOW);
	digitalWrite(INSTOUT, LOW);
	digitalWrite(CLKOUT, HIGH);
	digitalWrite(CLKOUT, LOW);

	for (i = 0; i < 8; i++) {

		if (bitOrder == LSBFIRST) {
			digitalWrite(DATAOUT, !!(val & (1 << i)));
			digitalWrite(INSTOUT, !!(val1 & (1 << i)));
		}
		else {
			digitalWrite(DATAOUT, !!(val & (1 << (7 - i))));
			digitalWrite(INSTOUT, !!(val1 & (1 << (7 - i))));
		}
		inBits <<= 1;
		inBits |= digitalRead(DATAIN);
		digitalWrite(CLKOUT, HIGH);
		digitalWrite(CLKOUT, LOW);

	}

	//End bits
	digitalWrite(DATAOUT, LOW);
	digitalWrite(INSTOUT, LOW);
	digitalWrite(CLKOUT, HIGH);
	digitalWrite(CLKOUT, LOW);
	digitalWrite(CLKOUT, HIGH);
	digitalWrite(CLKOUT, LOW);

	return inBits;
}

// Returns the value of the HFUSE
int readFuses() {
	int inData = 0;        // incoming serial byte AVR
	Serial.println("Reading fuses");

	Serial.print("signature reads as");
	for (int b = 0; b <= 2; b++)
	{
		shiftOut(MSBFIRST, 0x08, 0x4C);
		shiftOut(MSBFIRST, b, 0x0C);
		shiftOut(MSBFIRST, 0x00, 0x68);
		inData = shiftOut(MSBFIRST, 0x00, 0x6C);
		Serial.print(" ");
		Serial.print(inData, HEX);
	}
	Serial.println();

	//Read lock
	shiftOut(MSBFIRST, 0x04, 0x4C);
	shiftOut(MSBFIRST, 0x00, 0x78);
	inData = shiftOut(MSBFIRST, 0x00, 0x7C);
	Serial.print("lock reads as ");
	Serial.println(inData, HEX);

	//Read lfuse
	shiftOut(MSBFIRST, 0x04, 0x4C);
	shiftOut(MSBFIRST, 0x00, 0x68);
	inData = shiftOut(MSBFIRST, 0x00, 0x6C);
	Serial.print("lfuse reads as ");
	Serial.println(inData, HEX);

	//Read hfuse
	shiftOut(MSBFIRST, 0x04, 0x4C);
	shiftOut(MSBFIRST, 0x00, 0x7A);
	inData = shiftOut(MSBFIRST, 0x00, 0x7E);
	Serial.print("hfuse reads as ");
	Serial.println(inData, HEX);
	int hFuse = inData;

	//Read efuse
	shiftOut(MSBFIRST, 0x04, 0x4C);
	shiftOut(MSBFIRST, 0x00, 0x6A);
	inData = shiftOut(MSBFIRST, 0x00, 0x6E);
	Serial.print("efuse reads as ");
	Serial.println(inData, HEX);
	Serial.println();

	return hFuse;
}
