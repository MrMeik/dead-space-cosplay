#include <Servo.h>
#include <FastLED.h>
#include <OneButton.h>
#include <Arduino.h>

#define NUM_LEDS  3
#define LED_PIN  8

CRGB leds[NUM_LEDS];

Servo rotator;                    // create rotator servo
Servo extender;                   // create extender servo

#define ROTATOR_PIN   4           // Servo Rotator Pin
#define EXTENDER_PIN  7           // Servo Extender Pin

#define TRIGGER_BUTTON_PIN 9      // Trigger Button Pin
#define ROTATOR_BUTTON_PIN  10    // Rotate Button Pin

                                  // Button used to control when the shields are extended and turn on the LEDs
OneButton triggerButton = OneButton(
  TRIGGER_BUTTON_PIN,
  false,                          // Button is Active Low
  false                           // Button does not need an internal pull up resistor
);

                                  // Button used to control the rotation of the emitter
OneButton rotatorButton = OneButton(
  ROTATOR_BUTTON_PIN,
  false,                          // Button is Active Low
  false                           // Button does not need an internal pull up resistor
);

#define VERTICAL 0                // Represents the vertical rotation state for the emitter
#define HORIZONTAL 1              // Represents the horizontal rotation state for the emitter

int rotatorCurrentState = VERTICAL;
int rotatorTargetState = VERTICAL;

#define RETRACTED 0               // Represents the retracted shield state
#define EXTENDED 1                // Represents the extended shield state

int extenderCurrentState = RETRACTED;
int extenderTargetState = RETRACTED;

#define EXTENDER_UPPER    180     // upper travel limit for extender servo
#define EXTENDER_LOWER    0       // lower travel limit for extender servo
#define ROTATOR_UPPER     110      // upper travel limit for rotator servo
#define ROTATOR_LOWER     5       // lower travel limit for rotator servo

int pos;

int dt=100;

void setup() {
  Serial.begin(9600);
  extender.attach(EXTENDER_PIN);            // attach extender servo
  extender.write(EXTENDER_UPPER);           // ensure shields are extended in case the rotator needs to move
  delay(300);                               // wait for servo to get there
  rotator.attach(ROTATOR_PIN);              // attach rotator servo
  rotator.write(ROTATOR_LOWER);             // move rotator to vertical
  delay(300);                               // wait for servo to get there
  extender.write(EXTENDER_LOWER);           // retract fins
  delay(300);                               // wait for servo to get there
  // At this point it is assumed that the shields are retracted and the emitter is vertical

  triggerButton.attachClick(triggerPressed);
  rotatorButton.attachClick(rotatorPressed);
  
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.setBrightness(50);
}

void loop() {
  if(extenderTargetState == EXTENDED) {
    if(extenderCurrentState != EXTENDED) {
      // Extend the shields
      for(pos = EXTENDER_LOWER; pos < EXTENDER_UPPER; pos += 1) 
      {                                   
        extender.write(pos);               
        delay(0);                        
      }


      extenderCurrentState = EXTENDED;
    }

    // At this point, shield is ensured to be extended

    // Only if the shields are extended will the emitter be rotated
    if(rotatorCurrentState != rotatorTargetState) {
      // Rotator should activate
      if(rotatorTargetState == HORIZONTAL) {
        // Rotate to horizontal
        for(pos = ROTATOR_LOWER; pos < ROTATOR_UPPER; pos += 1)
        {                                
          rotator.write(pos);              
          delay(2);                       
        } 

        rotatorCurrentState = HORIZONTAL;
      }
      else {
        // Rotate to vertical  
        for(pos = ROTATOR_UPPER; pos >= ROTATOR_LOWER; pos -= 1)
        {                                   
          rotator.write(pos);               
          delay(2);                        
        } 

        rotatorCurrentState = VERTICAL;
      }
    }
    else {
      // Current position is at target state
      if(rotatorTargetState == VERTICAL) {
        rotator.write(ROTATOR_LOWER);
      }
      else {
        rotator.write(ROTATOR_UPPER);
      }      
    }
  }
  else {
    // Target state is retracted
    if(extenderCurrentState != RETRACTED) {
      // Retract the shields
      for(pos = EXTENDER_UPPER; pos >= EXTENDER_LOWER; pos -= 1) 
      {                                   
        extender.write(pos);               
        delay(0);                        
      }
      
      extenderCurrentState = RETRACTED;
    }
  }
  
  // If the shields are fully extended, then the lights should be turned on
  if(extenderCurrentState == EXTENDED && extenderTargetState == EXTENDED){
    fill_solid(leds, NUM_LEDS, CRGB::Cyan);
  }
  else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }

  FastLED.show();
  triggerButton.tick();
  rotatorButton.tick();
}

static void triggerPressed() {
  // When the trigger is pressed, toggle the retraction state of the shield extender servo
  if(extenderTargetState == RETRACTED) extenderTargetState = EXTENDED;
  else extenderTargetState = RETRACTED;
}



static void rotatorPressed() {
  // If the shields are retracted, this functionality is disabled
  if(extenderCurrentState != EXTENDED || extenderTargetState != EXTENDED) 
    return;

  // Change target rotation of the emitter between horizontal and vertical
  if(rotatorTargetState == VERTICAL) rotatorTargetState = HORIZONTAL;
  else rotatorTargetState = VERTICAL;
}
