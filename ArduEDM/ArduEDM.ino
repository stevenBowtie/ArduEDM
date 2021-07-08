#include "./ADC/ADC.h"
#include "./TeensyStep/src/TeensyStep.h"
#include <AccelStepper.h>

#define STEP   4
#define DIR    5
#define EN     6
#define ACCEL  5
#define MSPEED 1000
#define PULSEWIDTH    20
#define UPDATEPERIOD  5000 //"Sufficient" per documentation

#define POT       A0
#define SPARK     A1
#define BUTT_UP   11
#define BUTT_DN   10
#define BUTT_TGL  12

Stepper zAxis( STEP, DIR );
RotateControl rc( PULSEWIDTH, UPDATEPERIOD );

IntervalTimer arcPulseTmr; //IntervalTimer uses PIT timer and is nicer to work with

int potVal;
int tgl_threshold = 49;
int tgl_count = 0;
bool tgl_state = 0;
bool tgl_state_prev = 0;

int spark_threshold = 40;
int pause_threshold = 14;
int short_threshold =  5;
float voltFactor = 3.65;
float voltage = 0;
unsigned long lastPrint = 0;

void setup(){
  pinMode( EN, OUTPUT );
  pinMode( BUTT_DN, INPUT_PULLUP );
  pinMode( BUTT_UP, INPUT_PULLUP );
  pinMode( BUTT_TGL, INPUT_PULLUP );
  digitalWrite( EN, 0 );  //EN is actually !EN
  Serial.begin( 115200 );
  zAxis.setAcceleration( ACCEL );
  zAxis.setMaxSpeed( MSPEED );
}

void loop(){
  check_pot();  
  check_tgl();
  voltage = get_voltage();
  if( ( millis() - lastPrint ) > 100 ){
    Serial.println( voltage );
  }
  if( tgl_state ){
    if( voltage > spark_threshold ){
      zAxis.move( 1 );
      zAxis.setSpeed( -1000 );
      //Serial.println( "Plunging" );
    }
    if( voltage < spark_threshold ){
      zAxis.move( 1 );
      zAxis.setSpeed( -20 );
      
    }
    if( voltage < pause_threshold ){
      zAxis.move( 1 );
      zAxis.setSpeed( 0 );
    } 
    if( voltage < short_threshold ){
      zAxis.move( -1 );
      zAxis.setSpeed( 1000 );
      //Serial.println( "Retracting" );
    }
      zAxis.run();
  }
  zAxis.run();
}

void check_depth(){
  //Add math to turn counts into real distance
  if( zAxis.getPosition() > maxDepth ){
    //Stop drilling cycle and retract
  }
}

void check_tgl(){
  tgl_count = ( !digitalRead( BUTT_TGL ) ) ? tgl_count+1 : tgl_count-1 ;
  tgl_count = constrain( tgl_count, 0, tgl_threshold+1 );
  if( tgl_state == tgl_state_prev && tgl_count > tgl_threshold ){
    tgl_state = !tgl_state;
  }
  if( !digitalRead( BUTT_TGL ) == 0 ){
    tgl_state_prev = tgl_state;
  }
}

void check_pot(){
  potVal = analogRead( POT );
  if( !digitalRead( BUTT_UP ) ){
    zAxis.setSpeed( potVal );
  }
  else if( !digitalRead( BUTT_DN ) ){
    zAxis.setSpeed( potVal * -1 );
  }
  else{
    zAxis.setSpeed( 0 );
  }
}

float get_voltage(){
  int reading = analogRead( SPARK );
  return reading/voltFactor; 
}
