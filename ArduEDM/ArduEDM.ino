#include <AccelStepper.h>
#define STEP 4
#define DIR  5
#define EN   6

#define POT       A0
#define BUTT_UP   11
#define BUTT_DN   10
#define BUTT_TGL  12

AccelStepper z_axis( AccelStepper::DRIVER, STEP, DIR );

int potVal;

void setup(){
  pinMode( EN, OUTPUT );
  pinMode( STEP, OUTPUT );
  pinMode( DIR, OUTPUT );
  pinMode( BUTT_DN, INPUT_PULLUP );
  pinMode( BUTT_UP, INPUT_PULLUP );
  pinMode( BUTT_TGL, INPUT_PULLUP );
  digitalWrite( EN, 0 );  //EN is actually !EN
  digitalWrite( DIR, 0 );
  Serial.begin( 115200 );
  z_axis.setAcceleration( 5 );
  z_axis.setMaxSpeed( 1000 );
}

void loop(){
  potVal = analogRead( POT );
  if( !digitalRead( BUTT_UP ) ){
    z_axis.setSpeed( potVal );
  }
  else if( !digitalRead( BUTT_DN ) ){
    z_axis.setSpeed( potVal * -1 );
  }
  else{
    z_axis.setSpeed( 0 );
  }
    z_axis.run();
  
}
