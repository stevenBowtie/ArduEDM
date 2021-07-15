#include "./ADC/ADC.h"
#include <TeensyStep.h>
#include <TeensyTimerTool.h>
//#include <AccelStepper.h>
//using namespace TeensyTimerTool;

#define STEP   4
#define DIR    5
#define EN     6
#define ACCEL  5000
#define MSPEED 1000
#define PULSEWIDTH    20
#define UPDATEPERIOD  5000 //"Sufficient" per documentation
#define SAMPLE_SIZE 100

#define POT       A22
#define SPARK     A21
#define IGBT      39
#define BUTT_UP   21
#define BUTT_DN   23
#define BUTT_TGL  22

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

int spark_servo_v = 45;
int spark_threshold = 40;
int pause_threshold = 14;
int short_threshold =  5;
int current_reading;
int pulse_on_time = 20;
int pulse_off_time = 20;
int sample_buffer[SAMPLE_SIZE];
int sample_count = 0;
int maxSpark = 0;
float servo_factor = 100;
float voltFactor = 3.65;
float voltage = 0;
unsigned long lastPrint = 0;
int maxDepth = 0;

void handle_ADC_ISR() {
  //Sample quickly into a buffer based on interrupt
  current_reading = arcADC->adc0->analogReadContinuous();
  sample_buffer[sample_count] = analogRead( SPARK );
  sample_count < SAMPLE_SIZE ? sample_count++ : sample_count = 0;
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
  if( !tgl_state){
    if( !digitalRead( BUTT_UP ) ){
      Serial.println( rc.getCurrentSpeed() );
      rc.overrideSpeed( potVal/1024.0 );
    }
    else if( !digitalRead( BUTT_DN ) ){
      rc.overrideSpeed( potVal/-1024.0 );
    }
    else{
      rc.overrideSpeed( 0 );
    }
  }
}

float get_voltage(){
  int reading = analogRead( SPARK );
  return reading/voltFactor; 
}


void updateMaxReading(){
  maxSpark = 0;
  for( int i=0; i < SAMPLE_SIZE; i++ ){
    if( sample_buffer[i] > maxSpark ){ maxSpark = sample_buffer[i]; }
  }
  maxSpark = maxSpark * voltFactor;
}

void servo_handle(){
  updateMaxReading();
  float overrideFactor = (maxSpark - spark_servo_v) / servo_factor;
  rc.overrideSpeed( overrideFactor );
}

void setup(){
  pinMode( LED_BUILTIN, OUTPUT );
  digitalWrite( LED_BUILTIN, 1);
  pinMode( EN, OUTPUT );
  pinMode( BUTT_DN, INPUT_PULLUP );
  pinMode( BUTT_UP, INPUT_PULLUP );
  pinMode( BUTT_TGL, INPUT_PULLUP );
  digitalWrite( EN, 0 );  //EN is actually !EN
  Serial.begin( 115200 );
  zAxis.setAcceleration( ACCEL );
  zAxis.setMaxSpeed( MSPEED );
  //rc.overrideSpeed( 0 );
  rc.rotateAsync( zAxis );
  arcADC->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);
  arcADC->adc0->enableInterrupts( handle_ADC_ISR );
  pulseTimer.begin(handle_timer_isr);
  delay(100);
}

void loop(){
  digitalWrite( LED_BUILTIN, tgl_state);
  check_pot();  
  check_tgl();
  if( tgl_state ){
    servo_handle();
  }
}

/*
Turn on transistor
if it is above the sparkover threshold drive forward
if it transitioned from sparkover to arc start timer 
if it is short circuit back up
*/
