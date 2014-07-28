
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 

RF24 radio(9,10);

#define RIGHTPIN A0
#define LEFTPIN A1

#define SHIFTDATA 2
#define SHIFTCLOCK 4
#define SHIFTLATCH 3

// global variable to store distance in
int distance = 44;

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 
  0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// seven segment character codes
//                 0,  1,  2,   3,  4,   5,   6,  7,    8,   9
byte digit[10] = {
  126, 72, 61, 109, 75, 103, 115,  76, 127, 111};


//
// Role management
//

void setup(void)
{
  //
  // Print preamble
  //
  pinMode(RIGHTPIN, INPUT);
  digitalWrite(RIGHTPIN, LOW);
  pinMode(LEFTPIN, INPUT);
  digitalWrite(LEFTPIN, LOW);
  Serial.begin(57600);
  printf_begin();

  //
  // Setup and configure rf radio
  //
  radio.begin();

  // optionally, increase the delay between retries & # of retries
//  radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
//  radio.setPayloadSize(8);
  radio.enableDynamicPayloads();

  //
  // Open pipes to other nodes for communication
  //
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);


  radio.setChannel(10);
  //
  // Dump the configuration of the rf unit for debugging
  //

  radio.printDetails();

  // set up shift register pins
  // serial data
  pinMode(SHIFTDATA, OUTPUT);
  digitalWrite(SHIFTDATA, LOW);
  // serial clock
  pinMode(SHIFTCLOCK, OUTPUT);
  digitalWrite(SHIFTCLOCK, LOW);
  // latch
  pinMode(SHIFTLATCH, OUTPUT);
  digitalWrite(SHIFTLATCH, LOW);
  digitalWrite(SHIFTLATCH, HIGH);
  updateDisplay();
}

void shiftData(byte b) {
  digitalWrite(SHIFTLATCH, LOW);
  // loop through bits 0 - 7 of byte b 
  for (int i=7; i>=0; i--) {
    // set data to high or low depending on bit
    // google 'bitwise and' to see what '&' does
    // google 'bit shift' to see what << does
    // if evaluates true if not zero, false if zero
    if (b & (1<<i)) {
      digitalWrite(SHIFTDATA, HIGH);
      Serial.print(1);
    }
    else {
      digitalWrite(SHIFTDATA, LOW);
      Serial.print(0);
    }
    // pulse the clock line
    digitalWrite(SHIFTCLOCK, HIGH);
    digitalWrite(SHIFTCLOCK, LOW);

    //    digitalWrite(SHIFTDATA, LOW);
  } 
  // latch the data from the shift register to the output register
  // pulse latch
  digitalWrite(SHIFTLATCH, HIGH);
  Serial.println();
}

char dir = 'S';
char prevDir = 'S';
void loop(void) {
  radio.stopListening();
  int rightButton = digitalRead(RIGHTPIN);
  int prevRightButton = rightButton;
  int leftButton = digitalRead(LEFTPIN);
  int prevLeftButton = leftButton;
  delay(30);
  rightButton = digitalRead(RIGHTPIN);
  leftButton = digitalRead(LEFTPIN);
  if( rightButton == prevRightButton && rightButton > 0) {
    if( leftButton == prevLeftButton && leftButton > 0)
      dir = 'F';
    else
      dir = 'R';
  }
  else if( leftButton == prevLeftButton && leftButton > 0) {
    dir = 'L';
  }
  else {
    dir = 'S';
  }
  if (dir != prevDir){
    printf("Now sending %c\n\r", dir);
    bool ok = radio.write( &dir, sizeof(char) );
    ok = radio.write( &dir, sizeof(char) );
    prevDir = dir;
    if( dir == 'S')
      getDistance();
  }
}

void getDistance() {
  printf("getting distance from car...\n\r");
  radio.startListening();
  // wait for data
  long timeout = millis();
  while( !radio.available() ){
    // check for timeout on distance
    if(millis() - timeout > 300){
      printf("TIMEOUT getting distance\n\r");
      break;
    }
  }
  // Dump the payloads until we've gotten everything
  bool done = false;
  while (!done)
  {
    printf("getting transmission\n\r...");
    // Fetch the payload, and see if this was the last one.
    done = radio.read( &distance, sizeof(int) );

    // Spew it
    printf("Got payload: %d...\n\r",distance);
    updateDisplay();
  }
}

void updateDisplay() {
  if( distance > 99)
    distance == 99;
  int tens = floor(distance/10);
  int ones = distance - tens*10;
  shiftData(digit[ones]);
  shiftData(digit[tens]);
}
