#include <AccelStepper.h>
#define STEP 4
#define DIR  5
#define EN   6

#define POT       A0
#define SPARK     A1
#define BUTT_UP   11
#define BUTT_DN   10
#define BUTT_TGL  12

AccelStepper z_axis( AccelStepper::DRIVER, STEP, DIR );

int potVal;
int tgl_threshold = 49;
int tgl_count = 0;
bool tgl_state = 0;
bool tgl_state_prev = 0;

int spark_threshold = 30;
int short_threshold =  5;
float voltFactor = 3.65;
float voltage = 0;
unsigned long lastPrint = 0;

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
  check_pot();  
  check_tgl();
  voltage = get_voltage();
  if( ( millis() - lastPrint ) > 100 ){
    Serial.println( voltage );
  }
  if( tgl_state ){
    if( voltage > spark_threshold ){
      z_axis.move( 1 );
      z_axis.setSpeed( -100 );
      z_axis.run();
      //Serial.println( "Plunging" );
    }
    if( voltage < short_threshold ){
      z_axis.move( -1 );
      z_axis.setSpeed( 1000 );
      //Serial.println( "Retracting" );
    }
  }
  z_axis.run();
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
    z_axis.setSpeed( potVal );
  }
  else if( !digitalRead( BUTT_DN ) ){
    z_axis.setSpeed( potVal * -1 );
  }
  else{
    z_axis.setSpeed( 0 );
  }
}

float get_voltage(){
  int reading = analogRead( SPARK );
  return reading/voltFactor; 
}
