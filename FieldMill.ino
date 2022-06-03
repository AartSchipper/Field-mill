// Field mill Arduino Micro firmware
// AS 05-2022
// Uses the TimerInterrupt library

// Scope output for debugging the hardware using the Arduino plotter
// #define DEBUG_SCOPE

// Calibrated offset and sensitivity to Volt / meter 
const int OFFSET = 160 ;          // integrated adc ticks
const float SENSITIVITY = 3.66;   // V/m integrated adc tick
const int Nperiods = 11;          // Numper of signal periods for one measurement
  
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0
#define USE_TIMER_1 true          // Needs to be set, otherwise TimerInterrupt will not work. 

#include "TimerInterrupt.h"

#define TIMER1_INTERVAL_MS    1

volatile static long Signal_A;
volatile static long Signal_B;
volatile static bool flagReady; 
volatile static bool flagOverload; 

void TimerHandler() {
  int InputA = analogRead(A0);  
  int InputB = analogRead(A1);
  bool rotor = digitalRead(10); 
  
  static bool overload = 0; 
  static bool lastRotor = 0; 
  static int state = 1; 
  static int periods = 0 ; 

#ifdef DEBUG_SCOPE
  Serial.print ("A:"); Serial.print(InputA); 
  Serial.print(","); 
  Serial.print ("B:"); Serial.print(InputB);
  Serial.print(",");  
  Serial.print ("Rotor:"); Serial.println(rotor * 1024); 
#else

  switch (state) { 
    case 1:
      // wait for start position and ready flag
      if (rotor == 1 && lastRotor == 0 && flagReady == 0) {
        Signal_A = 0; 
        Signal_B = 0; 
        state = 2; 
      }
    break; 
    
    case 2:
      // Collect and rectify
      if (rotor) { 
        Signal_A -= InputA;
        Signal_B += InputB;
      } else { 
        Signal_A += InputA;
        Signal_B -= InputB;
      }
      
      if (InputA > 1022 || InputB > 1022 || InputA < 1 || InputB < 1) flagOverload  = 1; // Check for clipping
      
      if (rotor == 1 && lastRotor == 0) state = 3;  
    break; 

    case 3: 
      if (++periods > Nperiods) state = 4; else state = 2; 
    break; 
    
    case 4: 
      periods = 0; 
      flagReady = 1; 
      state  = 1; 
    break;
  }
  
  lastRotor = rotor;  
  
#endif
}

void setup() {
  // put your setup code here, to run once:
  pinMode(A0, INPUT); // A input
  pinMode(A1, INPUT); // B input
  
  pinMode(10, INPUT_PULLUP); // Tacho pin
  
  digitalWrite(9, LOW); //PWM pin, just make low for minimal speed
  pinMode(9, OUTPUT); 
  
  Serial.begin(115200);   // USB serial output
 
  // Set a 1 ms interrupt on Timer3. 
  ITimer1.init();

  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS, TimerHandler)) {
    Serial.print(F("Starting  ITimer1 OK, millis() = ")); Serial.println(millis());
  } else {
    Serial.println(F("Can't set ITimer1. Select another freq. or timer"));
  }
  
  // label for plotter
  Serial.println("V/m:"); 
}

void loop() {
#ifdef DEBUG_SCOPE 
  // do nothing
#else
  // Output is data is available
  if (flagReady) { 
    float outputVm = ((float)((Signal_A + Signal_B)/Nperiods) - OFFSET) * SENSITIVITY;
    if (flagOverload) outputVm = 0; 
    Serial.println(outputVm); 
    
    flagReady = 0; 
    flagOverload =0; 
  }
#endif

}
