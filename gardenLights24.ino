
//for the nanos - use:
//    arduino nano
//    atmega328p (old bootloader) 
//    com5 (RH rear USB)
//
//    green   D6
//    blue    A4
//    orange  5V
//    yellow  GND

#include <FastLED.h>

//LED definitions
#define LED_PIN     6
#define COLOR_ORDER RGB
#define CHIPSET     WS2811
#define NUM_LEDS    50
CRGB leds[NUM_LEDS];
int theHue = 0;           //fixed hue, changed each time the simulation is 'poked'

//simulation definitions
int y;
float avg;
float speedMult = 0.04;
float f[NUM_LEDS];
float df[NUM_LEDS];
//sine function for perturbation (could be half the size since symmetrical but let's see if memory is a problem)
float p[9] = {0.5729488,2.072949,3.927051,5.4270506,6.0,5.4270506,3.9270515,2.072949,0.57294935};

//display variables & dark detection
#define LDR_PIN A4
#define DISPPIN 13
#define THRESHOLD 0.01
int darkCheckDelay = 1000;
int darkFor = 30000;    //has to be dark for this many milliseconds before we declare it dark
boolean dark = false;   //bright = low ~100, dark = high ~1011 on A4 pin (LDR_PIN)
int darkVal = 20;      //comparison value for darkness, pinVal > darkVal => dark = true
int pinVal = 0;         //value read from pin

boolean runner = false; //switch for if the state is running on timer or not, false when timer runs out, true when first dark

//timer variables
int cycleDelayLo = 15000;
int cycleDelayHi = 25000;
boolean timerRunning = false;
unsigned long startMillis;    //start timer used for run time
unsigned long currentMillis;  //used in the time check for the overall run time
const unsigned long period = 1000L*60*60*1;  //max period to operate for (1hr) one of the components must explicitly be a long (1000L)

boolean runSim = false;

int hueLo = 0;
int hueHi = 255;

void setup() {
//  Serial.begin(9600);
  //setup pins
  pinMode(LDR_PIN,INPUT);   //analog pin input for LDR, connected to voltage divider & current limiting resistor
  pinMode(DISPPIN,OUTPUT);  // pin 13 is connected to an LED on the board to show if dark is detected (useful for debug)

  //setup LEDs
  delay(3000); // LED sanity delay
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
}

void loop() {
  //check for dark and run if dark
  if (dark) {
    //do a simulation step if dark
    if (runSim) {
      calcAndShow();
    } else {
      //not running sim, wait for a delay to trigger the poke and start the simulation
      delay(random(cycleDelayLo,cycleDelayHi));
      
      setHueLoAndHi();
      poke();
      
      runSim = true;
      //proof of life blink for LED
      digitalWrite(DISPPIN, LOW);
      delay(200);
      digitalWrite(DISPPIN, HIGH);
    }
    //end of dark section
  } else {
    //not dark, check if dark for long enough
    int startCheck = millis();  //timer for the dark
    int darkTimer = 0;
    
    while (!dark) {
      pinVal = analogRead(LDR_PIN); //check the light level
      if (pinVal<=darkVal) {
        //dark, increment the timer
        darkTimer = millis()-startCheck;
        
        if (darkTimer >= darkFor) {
          dark = true;
          digitalWrite(DISPPIN, LOW);
          delay(200);
          digitalWrite(DISPPIN, HIGH);
          delay(200);
          digitalWrite(DISPPIN, LOW);
          delay(200);
          digitalWrite(DISPPIN, HIGH);
          delay(200);
          digitalWrite(DISPPIN, LOW);
          delay(200);
          digitalWrite(DISPPIN, HIGH);  //turn on the LED to show it as dark - ignores the timer
        }
        
      } else {
        //not dark, reset the timer and wait for a bit
        dark = false;
        darkTimer = 0;
        delay(darkCheckDelay);
        digitalWrite(DISPPIN, LOW);   //turn off the LED to show light
      }
    }
  }
}

void setHueLoAndHi() {
  int hueRange = int(random(50,205));
  hueLo = int(random(0,hueRange));
  hueHi = int(random(hueRange,255));
}

void calcAndShow() {
  //loop through whole array: f=f+df
  y = NUM_LEDS;
  while(y-->0) f[y]+=df[y]*speedMult;
  
  //loop through centre of array (miss out both ends to avoid overflow): df=df+ddf
  y = NUM_LEDS-1;
  while(y-->1) {
    avg=(f[y-1]+f[y+1])/2;
    df[y]-=(f[y]-avg);
    df[y]=df[y]*0.997;
  }
  
  //check for max/min values
  int maxVal = f[0];
  int minVal = f[0];
  y = NUM_LEDS;
  while(y-->1) {
    if(f[y]>maxVal) maxVal = f[y];
    if(f[y]<minVal) minVal = f[y];
  }
  //squelch if range high-low is below threshold and stop simulation
  if ((abs(maxVal)+abs(minVal))<(THRESHOLD)) {
    //value is low enough to stop the sim
    y = NUM_LEDS;
    while(y-->0) {
      f[y]=0;
      df[y]=0;
    }
    runSim = false;
    //set the lights to off
    y = NUM_LEDS;
    while(y-->0) leds[y] = CRGB::Black;
    //check for dark, see if the lights need to stop
    pinVal = analogRead(LDR_PIN); //check the light level
    if (pinVal>darkVal) dark = false;
    //check the timer, see if it's run for long enough
  } else {
    //f now contains the simulation values to write to the lights
    for (int i=0;i<NUM_LEDS;i++) {
      //map both hue and brightness to value of f
      leds[i] = CHSV(int(constrain(map(f[i],-3,3,0,255),hueLo,hueHi)),255,int(constrain(map(abs(f[i]),0,3,0,255),0,255)));
    }
  }
  FastLED.show();
}

void poke() {
  //imposes an input on the f array, based on a cosine function for shape
  int pokeIndex = int(random(0,NUM_LEDS-9-1));
  for (int i=0;i<9;i++) {
    //loop through poke array, adding to f array
    f[i+pokeIndex] += p[i];
  }
}
