#include "./ADC/ADC.h"
#include <TeensyStep.h>
#include <TeensyTimerTool.h>
//#include <AccelStepper.h>
//using namespace TeensyTimerTool;

#define STEP   4
#define DIR    5
#define EN     6
#define ACCEL  5
#define MSPEED 1000
#define PULSEWIDTH    20
#define UPDATEPERIOD  5000 //"Sufficient" per documentation

#define POT       A0
#define SPARK     A1
#define IGBT      39
#define BUTT_UP   11
#define BUTT_DN   10
#define BUTT_TGL  12

Stepper zAxis( STEP, DIR );
RotateControl rc( PULSEWIDTH, UPDATEPERIOD );
ADC *arcADC = new ADC();

TeensyTimerTool::OneShotTimer pulseTimer;
//IntervalTimer arcPulseTmr; //IntervalTimer uses PIT timer and is nicer to work with

int potVal;
int tgl_threshold = 49;
int tgl_count = 0;
bool tgl_state = 0;
bool tgl_state_prev = 0;
bool pulseTransitionFlag = 0;
bool igbt_next_state = 0;

int spark_threshold = 40;
int pause_threshold = 14;
int short_threshold =  5;
int current_reading;
int pulse_on_time = 20;
int pulse_off_time = 20;
float voltFactor = 3.65;
float voltage = 0;
unsigned long lastPrint = 0;
int maxDepth = 0;

void setup(){
  pinMode( EN, OUTPUT );
  pinMode( BUTT_DN, INPUT_PULLUP );
  pinMode( BUTT_UP, INPUT_PULLUP );
  pinMode( BUTT_TGL, INPUT_PULLUP );
  digitalWrite( EN, 0 );  //EN is actually !EN
  Serial.begin( 115200 );
  //zAxis.setAcceleration( ACCEL );
  //zAxis.setMaxSpeed( MSPEED );
  arcADC->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);
  arcADC->adc0->enableInterrupts( handle_ADC_ISR );
  pulseTimer.begin(handle_timer_isr);
}

void loop(){
  check_pot();  
  check_tgl();
  voltage = get_voltage();
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
    //zAxis.setSpeed( potVal );
  }
  else if( !digitalRead( BUTT_DN ) ){
    //zAxis.setSpeed( potVal * -1 );
  }
  else{
    //zAxis.setSpeed( 0 );
  }
}

float get_voltage(){
  int reading = analogRead( SPARK );
  return reading/voltFactor; 
}

void handle_ADC_ISR() {
    current_reading = arcADC->adc0->analogReadContinuous();
    if (current_reading < short_threshold) {
        //too far
    }
    if (current_reading < spark_threshold ) {
        //Begin arc
        if (!pulseTransitionFlag) {
            pulseTimer.trigger(pulse_on_time);
            pulseTransitionFlag = 1;
        }

    }
    if (current_reading > spark_threshold) {
        //Drive forward
        pulseTransitionFlag = 0;
    }
}

void handle_timer_isr() {
    if (igbt_next_state) {
        //Turn transistor off
        digitalWriteFast( IGBT, 0 );
        igbt_next_state = 1;
        pulseTimer.trigger(pulse_off_time);
    }
    else {
        //Turn the transistor on
        digitalWriteFast(IGBT, 1);
        igbt_next_state = 0;
    }
}

/*
Turn on transistor
if it is above the sparkover threshold drive forward
if it transitioned from sparkover to arc start timer 
if it is short circuit back up
*/