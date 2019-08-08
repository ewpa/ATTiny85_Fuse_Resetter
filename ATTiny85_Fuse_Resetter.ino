#include "Arduino.h"
// AVR High-voltage Serial Programmer
// Originally created by Paul Willoughby 03/20/2010
// www.rickety.us slash 2010/03/arduino-avr-high-voltage-serial-programmer/
// Inspired by Jeff Keyzer mightyohm.com
// Serial Programming routines from ATtiny25/45/85 datasheet
// Hardware interface by Ewan Parker

// Device signatures that are understood
#define SIG_ATTINY25 0x1E9108
#define SIG_ATTINY45 0x1E9206
#define SIG_ATTINY85 0x1E930B

// The fuse types we recognize
typedef enum fuseType
{
  FUSE_T_LOCK,         // Lock bits
  FUSE_T_HFUSE,        // High fuse bits
  FUSE_T_LFUSE,        // Low fuse bits
  FUSE_T_EFUSE,        // Extended fuse bits
  FUSE_T_SIZEOF
};

// The three states we allow fuses to be set
typedef enum fuseState
{
  FUSE_S_RESET_EN = 1, // Pin 1 as RESET, otherwise unchanged
  FUSE_S_RESET_GPIO,   // Pin 1 as GPIO, otherwise unchanged
  FUSE_S_DEFAULT,      // All fuses as factory default
  FUSE_S_END
};

#define  RST     13    // Output to level shifter for !RESET from NPN to Pin 1
#define  CLKOUT   9    // Connect to Serial Clock Input (SCI) Pin 2
#define  DATAIN  10    // Connect to Serial Data Output (SDO) Pin 7
#define  INSTOUT 11    // Connect to Serial Instruction Input (SII) Pin 6
#define  DATAOUT 12    // Connect to Serial Data Input (SDI) Pin 5
#define  VCC      8    // Connect to VCC Pin 8

#define  ALERT   A5    // Indicator pin
#define  PROG    A4    // Programming button inout pin

// Fuse configurations (Defaults for ATtiny25/45/85)
#define FUSE_MASK_RSTDISBL 0x80
fuseType defaultFuses[FUSE_T_SIZEOF] = { 0xFF, 0xDF, 0x62, 0xFE };
fuseType actualFuses[FUSE_T_SIZEOF], targetFuses[FUSE_T_SIZEOF];

void setup()
{
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

  // Programming pin
  pinMode(PROG, INPUT_PULLUP);

  // Buzzer / beeper / LED
  pinMode(ALERT, OUTPUT);
  delay(200);
  digitalWrite(ALERT, LOW);
  delay(1000);

  // start serial port at 9600 bps:
  Serial.begin(9600);
}

void loop()
{
  Serial.println("Turn on the 12 volt power\n");
  Serial.println("Entering programming Mode\n");

  // Initialize pins to enter programming mode
  pinMode(DATAIN, OUTPUT);  //Temporary
  digitalWrite(DATAOUT, LOW);
  digitalWrite(INSTOUT, LOW);
  digitalWrite(DATAIN, LOW);
  digitalWrite(RST, HIGH);  // Level shifter is inverting, so shuts off 12V

  // Enter High-voltage Serial programming mode
  digitalWrite(VCC, HIGH);  // Apply VCC to start programming process
  delayMicroseconds(20);
  digitalWrite(RST, LOW);   //Turn on 12v
  delayMicroseconds(10);
  pinMode(DATAIN, INPUT);   //Release DATAIN
  delayMicroseconds(300);

  //Programming mode
  uint32_t signature = readSignature();
  if (signature != SIG_ATTINY85 && signature != SIG_ATTINY45
  && signature != SIG_ATTINY25)
  {
    fatalError("Unknown device signature");
  }

  readFuses(actualFuses);   // Read the fuses and populate array

  switch (establishContact())
  {
    case FUSE_S_RESET_EN :
      for (int f = 0; f < FUSE_T_SIZEOF; f++) targetFuses[f] = actualFuses[f];
      targetFuses[FUSE_T_HFUSE] |= FUSE_MASK_RSTDISBL;
      break;
    case FUSE_S_RESET_GPIO :
      for (int f = 0; f < FUSE_T_SIZEOF; f++) targetFuses[f] = actualFuses[f];
      targetFuses[FUSE_T_HFUSE]
        = targetFuses[FUSE_T_HFUSE] & ~FUSE_MASK_RSTDISBL;
      break;
    case FUSE_S_DEFAULT :
      for (int f = 0; f < FUSE_T_SIZEOF; f++) targetFuses[f] = defaultFuses[f];
      break;
    default:
      for (int f = 0; f < FUSE_T_SIZEOF; f++) targetFuses[f] = actualFuses[f];
  }

  Serial.println("");

  //Write fuse if not already the value we want

  Serial.print("lock  current=");
  Serial.print(actualFuses[FUSE_T_LOCK], HEX);
  Serial.print(" target=");
  Serial.print(targetFuses[FUSE_T_LOCK], HEX);
  if (actualFuses[FUSE_T_LOCK] != targetFuses[FUSE_T_LOCK])
  {
    //Write lock
    Serial.print(" updating...");
    shiftOut(MSBFIRST, 0x20, 0x4C);
    shiftOut(MSBFIRST, 0xFF, 0x2C);
    shiftOut(MSBFIRST, 0x00, 0x64);
    shiftOut(MSBFIRST, 0x00, 0x6C);
    Serial.println("done");
  }
  else
  {
    Serial.println("");
  }

  Serial.print("efuse current=");
  Serial.print(actualFuses[FUSE_T_EFUSE], HEX);
  Serial.print(" target=");
  Serial.print(targetFuses[FUSE_T_EFUSE], HEX);
  if (actualFuses[FUSE_T_EFUSE] != targetFuses[FUSE_T_EFUSE])
  {
    //Write efuse
    Serial.print(" updating...");
    shiftOut(MSBFIRST, 0x40, 0x4C);
    shiftOut(MSBFIRST, 0xFF, 0x2C);
    shiftOut(MSBFIRST, 0x00, 0x66);
    shiftOut(MSBFIRST, 0x00, 0x6E);
    Serial.println("done");
  }
  else
  {
    Serial.println("");
  }

  Serial.print("hfuse current=");
  Serial.print(actualFuses[FUSE_T_HFUSE], HEX);
  Serial.print(" target=");
  Serial.print(targetFuses[FUSE_T_HFUSE], HEX);
  if (actualFuses[FUSE_T_HFUSE] != targetFuses[FUSE_T_HFUSE])
  {
    Serial.print(" updating...");
    shiftOut(MSBFIRST, 0x40, 0x4C);
    shiftOut(MSBFIRST, targetFuses[FUSE_T_HFUSE], 0x2C);
    shiftOut(MSBFIRST, 0x00, 0x74);
    shiftOut(MSBFIRST, 0x00, 0x7C);
    Serial.println("done");
  }
  else
  {
    Serial.println("");
  }

  Serial.print("lfuse current=");
  Serial.print(actualFuses[FUSE_T_LFUSE], HEX);
  Serial.print(" target=");
  Serial.print(targetFuses[FUSE_T_LFUSE], HEX);
  if (actualFuses[FUSE_T_LFUSE] != targetFuses[FUSE_T_LFUSE])
  {
    //Write lfuse
    Serial.print(" updating...");
    shiftOut(MSBFIRST, 0x40, 0x4C);
    shiftOut(MSBFIRST, targetFuses[FUSE_T_LFUSE], 0x2C);
    shiftOut(MSBFIRST, 0x00, 0x64);
    shiftOut(MSBFIRST, 0x00, 0x6C);
    Serial.println("done");
  }
  else
  {
    Serial.println("");
  }

  // Confirm new state of play
  Serial.println("");
  readFuses(actualFuses);

  digitalWrite(CLKOUT, LOW);
  digitalWrite(VCC, LOW);
  digitalWrite(RST, HIGH);   //Turn off 12v

  // Indicate that we are done
  digitalWrite(ALERT, HIGH);
  delay(200);
  digitalWrite(ALERT, LOW);
  delay(200);
  digitalWrite(ALERT, HIGH);

  Serial.println("Programming complete. Press RESET to run again.");
  while (true) {}
}

char establishContact()
{
  digitalWrite(ALERT, HIGH);
  Serial.println("You can ENABLE the RST pin (as RST) to allow programming");
  Serial.println("or DISABLE it to turn it into a (weak) GPIO pin.\n");
  Serial.println("Press the desired number or hold the button for the desired number of seconds.");

  // We must get a valid choice to proceed
  unsigned char reply;

  do
  {
    Serial.println();
    Serial.println("1 second  : ENABLE the RST pin (back to normal)");
    Serial.println("2 seconds : DISABLE the RST pin (make it a GPIO pin)");
    Serial.println("3 seconds : Revert to factory defaults");

    int buttonPressedMs = 0;
    bool buttonState, buttonReleased = false;
    while (!Serial.available() && !buttonReleased)
    {
      // Wait for user input
      delay(20);
      buttonState = !digitalRead(PROG);
      if (buttonPressedMs > 0) digitalWrite(ALERT, LOW);
      if (!buttonState && buttonPressedMs > 0)
        buttonReleased = true;
      else buttonPressedMs += buttonState * 20;
      if (buttonPressedMs%1000 == 0 && buttonPressedMs > 0)
      {
        Serial.print(buttonPressedMs/1000);
        Serial.print("...");
        digitalWrite(ALERT, HIGH);
      }
    }

    if (buttonPressedMs >= 1000)
    {
      reply = buttonPressedMs/1000;
      Serial.println();
    }
    else if (Serial.available()) reply = Serial.read() - '0';
    else reply = 0;
  }
  while (!(reply >= 1 && reply < FUSE_S_END));

  digitalWrite(ALERT, LOW);
  return reply;
}

int shiftOut(uint8_t bitOrder, byte val, byte val1)
{
  int i;
  int inBits = 0;
  //Wait until DATAIN goes high
  while (!digitalRead(DATAIN)) {}

  //Start bit
  digitalWrite(DATAOUT, LOW);
  digitalWrite(INSTOUT, LOW);
  digitalWrite(CLKOUT, HIGH);
  digitalWrite(CLKOUT, LOW);

  for (i = 0; i < 8; i++)
  {
    if (bitOrder == LSBFIRST)
    {
      digitalWrite(DATAOUT, !!(val & (1 << i)));
      digitalWrite(INSTOUT, !!(val1 & (1 << i)));
    }
    else
    {
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

// Returns the value of the device signature
uint32_t readSignature()
{
  uint32_t signature = 0x00;
  int inData = 0;        // incoming serial byte AVR
  Serial.print("Reading signature...");

  for (int b = 0; b <= 2; b++)
  {
    shiftOut(MSBFIRST, 0x08, 0x4C);
    shiftOut(MSBFIRST, b, 0x0C);
    shiftOut(MSBFIRST, 0x00, 0x68);
    inData = shiftOut(MSBFIRST, 0x00, 0x6C);
    signature = (signature << 8) + inData;
  }
  Serial.println(signature, HEX);
  return signature;
}

// Populates the values of the fuses and lock bits
void readFuses(fuseType fuses[])
{
  int inData = 0;        // incoming serial byte AVR
  Serial.println("Reading locks and fuses");

  //Read lock
  Serial.print("lock....");
  shiftOut(MSBFIRST, 0x04, 0x4C);
  shiftOut(MSBFIRST, 0x00, 0x78);
  inData = shiftOut(MSBFIRST, 0x00, 0x7C);
  fuses[FUSE_T_LOCK] = inData;
  Serial.println(fuses[FUSE_T_LOCK], HEX);

  //Read lfuse
  Serial.print("lfuse...");
  shiftOut(MSBFIRST, 0x04, 0x4C);
  shiftOut(MSBFIRST, 0x00, 0x68);
  inData = shiftOut(MSBFIRST, 0x00, 0x6C);
  fuses[FUSE_T_LFUSE] = inData;
  Serial.println(fuses[FUSE_T_LFUSE], HEX);

  //Read hfuse
  Serial.print("hfuse...");
  shiftOut(MSBFIRST, 0x04, 0x4C);
  shiftOut(MSBFIRST, 0x00, 0x7A);
  inData = shiftOut(MSBFIRST, 0x00, 0x7E);
  fuses[FUSE_T_HFUSE] = inData;
  Serial.println(fuses[FUSE_T_HFUSE], HEX);

  //Read efuse
  Serial.print("efuse...");
  shiftOut(MSBFIRST, 0x04, 0x4C);
  shiftOut(MSBFIRST, 0x00, 0x6A);
  inData = shiftOut(MSBFIRST, 0x00, 0x6E);
  fuses[FUSE_T_EFUSE] = inData;
  Serial.println(fuses[FUSE_T_EFUSE], HEX);

  Serial.println();
}

// A fatal error from which the program never returns.
void fatalError(String msg)
{
  Serial.println("*** Fatal error ***");
  Serial.println(msg);
  Serial.println("RESET to restart");
  while (true)
  {
    digitalWrite(ALERT, !digitalRead(ALERT));
    delay(200);
  }
}
