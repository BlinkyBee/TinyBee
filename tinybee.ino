#define NO_CORRECTION 1
//#define FASTLED_DOUBLE_OUTPUT 1
#include <FastLED.h>

#define NUM_LEDS 64
#define DATA_PIN 6
#define SWITCH_PIN 7

//--------------------------------------------------------------------------------------------------
// globals
//--------------------------------------------------------------------------------------------------

// rotary encoder state. TODO: Make this a class
bool g_pressed = false;
bool g_pressedThisFrame = false;
bool g_releasedThisFrame = false;
uint32_t g_pressTime = 0;
uint32_t g_releaseTime = 0;

// volatile because it'll be updated from ISR
volatile int16_t g_rotenc = 200;

CRGB leds[NUM_LEDS];

uint8_t g_hue = 0;

//--------------------------------------------------------------------------------------------------
// patterns
//--------------------------------------------------------------------------------------------------

void rainbow() {
    // FastLED's built-in rainbow generator
    fill_rainbow( leds, NUM_LEDS, g_hue, 7);
}


void confetti() {
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy( leds, NUM_LEDS, 7);
    if (random8() > 100) {
      int pos = random8(NUM_LEDS);
      leds[pos] += CHSV( g_hue + random8(64), 200, 255);
    }
}


void sinelon() {
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( leds, NUM_LEDS, 20);
    int pos = beatsin16(11,0,NUM_LEDS);
    leds[pos] += CHSV( g_hue, 255, 192);
} 

const TProgmemPalette16 myPalette_p PROGMEM =
{
    CRGB::Blue,
    CRGB::Indigo,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black,
    
    CRGB::Pink,
    CRGB::Aqua,
    CRGB::Purple,
    CRGB::Aqua,
    CRGB::Black,
    CRGB::Black,    
    
    CRGB::Orange,
    CRGB::Purple,
    CRGB::Orange,
    CRGB::Black,
    CRGB::Black
};

void moving_palette() {
    static uint8_t start = 0;
    start++;
    FillLEDsFromPaletteColors(myPalette_p, start);
}

//--------------------------------------------------------------------------------------------------

void FillLEDsFromPaletteColors(CRGBPalette16 palette, uint8_t colorIndex)
{    
    for( int i = 0; i < NUM_LEDS; i++, colorIndex+=3) {
        leds[i] = ColorFromPalette( palette, colorIndex, 255, LINEARBLEND);
    }
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList patterns = { 
    confetti, 
    rainbow, 
    sinelon,
    moving_palette,
 };

uint8_t g_current_pattern = 0;
bool g_autocycle = true;

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void next_pattern()
{
    // add one to the current pattern number, and wrap around at the end
    g_current_pattern = (g_current_pattern + 1) % ARRAY_SIZE( patterns);
}


//--------------------------------------------------------------------------------------------------
// setup & loop
//--------------------------------------------------------------------------------------------------

void setup() {
    init_rotenc_pins();
     
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    pinMode(DATA_PIN, OUTPUT);
}

void loop() {
    FastLED.setBrightness(g_rotenc);
    
    read_switch();  
    if (g_releasedThisFrame) {
        next_pattern();
    }

    (patterns)[g_current_pattern]();
    FastLED.show();
    FastLED.delay(1000/120); 

    EVERY_N_MILLISECONDS( 20 ) { g_hue++; }
}

//--------------------------------------------------------------------------------------------------
// Rotary encoder stuff
//--------------------------------------------------------------------------------------------------

void init_rotenc_pins(){
  // set pins to input
  pinMode(12, INPUT); // A - PA6
  pinMode(13, INPUT); // B - PA7
  pinMode(SWITCH_PIN, INPUT_PULLUP); // Switch
  
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

void read_switch() {
    g_releasedThisFrame = false;
    g_pressedThisFrame = false;
    uint32_t now = millis();
    
    bool currentPressed = digitalRead(SWITCH_PIN) == LOW;
    if (currentPressed != g_pressed) {
      // release
      if (g_pressed) {
        g_releaseTime = now;
        g_pressed = false;
        g_releasedThisFrame = true;
      // press
      } else {
        // simple debounce
        if (now - g_releaseTime > 50) {
          g_pressTime = now;
          g_pressed = true;
          g_pressedThisFrame = true;
        }
      }
    }
  }
