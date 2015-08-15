#define NO_CORRECTION 1
#include <FastLED.h>

#define NUM_LEDS 30
#define DATA_PIN 6

volatile int16_t g_rotenc = 200;
CRGB leds[NUM_LEDS];

//--------------------------------------------------------------------------------------------------
// setup & loop
//--------------------------------------------------------------------------------------------------

void setup() {
  init_rotenc_pins(); 
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  pinMode(DATA_PIN, OUTPUT);
}

void loop() {
  FastLED.clear();
  for (int i=0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(i * 5, 255, 200);
  }
  FastLED.setBrightness(g_rotenc);
  FastLED.show();
}

//--------------------------------------------------------------------------------------------------
// Rotary encoder stuff
//--------------------------------------------------------------------------------------------------

void init_rotenc_pins(){
  // set pins to input
  pinMode(12, INPUT); // PA6
  pinMode(13, INPUT); // PA7

  // setup pin change interrupt
  cli();  
  PCICR = 0x01;
  PCMSK0 = 0b11000000;
  sei();
}

int8_t read_encoder()
{
  static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  static uint8_t old_AB = 0;
  
  old_AB <<= 2;                   
  old_AB |= (( PINA & 0b11000000 ) >> 6);
  return ( enc_states[( old_AB & 0x0f )]);
}

ISR(PCINT0_vect) {
  g_rotenc -= read_encoder();
  if (g_rotenc < 0) {
    g_rotenc = 0; 
  }
  if (g_rotenc > 255) {
    g_rotenc = 255;
  }
}

