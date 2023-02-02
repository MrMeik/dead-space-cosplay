#include <FastLED.h>

#define HEALTH_NUM_LEDS  14
#define HEALTH_LED_PIN   3

#define STASIS_NUM_LEDS  4
#define STASIS_LED_PIN   2

#define POT_PIN          A1

// Healthbar rig layout
// section 4: LEDs 0-2 
// section 3: LEDs 3-6 
// section 2: LEDs 7-9
// section 1: LEDs 10-13
// Section 1 is the base of the spine
// last led: 13

int section_led_count[] = {4, 3, 4, 3};

CRGB health_leds[HEALTH_NUM_LEDS];
CHSV hue_health_leds[HEALTH_NUM_LEDS];
CRGB stasis_leds[STASIS_NUM_LEDS];

int current_health  =    0;
int target_health   =    0;
int previous_target =    0;
int next_target     =    0;

#define HEALTH_BLOCK     25

#define YELLOW_THRESHOLD 50
#define RED_THRESHOLD    25

CRGB rgb_blue_color = CRGB(137, 215, 175);

CHSV blue_color = CHSV(85, 117, 255);//CHSV(202, 107, 255);
CHSV yellow_color = CHSV(24, 239, 255); 
CHSV red_color = CHSV(0, 255, 255);

void setup() {
  FastLED.addLeds<WS2812B, HEALTH_LED_PIN, GRB>(health_leds, HEALTH_NUM_LEDS);
  //FastLED.addLeds<WS2812B, STASIS_LED_PIN, GRB>(stasis_leds, STASIS_NUM_LEDS);
  FastLED.setBrightness(50);
  Serial.begin(9600);
  pinMode(POT_PIN, INPUT);
  uint16_t potRead = analogRead(POT_PIN);
  current_health = map(potRead, 20, 950, 0, 4) * HEALTH_BLOCK;
  target_health = current_health;
}

void loop() {

  FastLED.clear();

  do_healthbar();
  //do_stasis();
  
  FastLED.show();
}

void log_led_color() {
  Serial.print("r:");
  Serial.print(health_leds[10].r);
  Serial.print("g:");
  Serial.print(health_leds[10].g);
  Serial.print("b:");
  Serial.println(health_leds[10].b);
}

void do_stasis() {
   fill_solid(stasis_leds, STASIS_NUM_LEDS, rgb_blue_color);
}

void do_healthbar() {
  try_update_target();
  update_current_health();
  
  if(current_health == 0) {
    // No health
    draw_no_health();
  }
  else {
    // Has health
    draw_healthbar();
  }

  hsv2rgb_raw(hue_health_leds, health_leds, HEALTH_NUM_LEDS);
}

void try_update_target() {
  uint16_t potRead = analogRead(POT_PIN);
  next_target = map(potRead, 20, 950, 0, 4) * HEALTH_BLOCK;
  if(previous_target == next_target) {
    target_health = next_target;
  }
  previous_target = next_target;
}

void blank_healthbar() {
  fill_solid(hue_health_leds, HEALTH_NUM_LEDS, CHSV(0, 0, 0)); // Black out bar
}

int time_offset = 0;
int cached_full_from_led = -1;
int cached_remainder_value = -1;

bool is_cache_valid() {
  return cached_full_from_led != -1;
}

void clear_cache() {
  cached_full_from_led = -1;
  cached_remainder_value = -1;
}

void update_current_health() {
 // TODO: consider if this should be every n seconds
  if(current_health != target_health) {
    time_offset += 1;

    if(time_offset == 7){
      time_offset = 0;
    
      if(current_health < target_health) {
        current_health += 1;
        clear_cache();
      }
      else {
        current_health -= 1;
        clear_cache();
      }
    }
  }
}

void draw_healthbar() {
  blank_healthbar();

  CHSV fill_color = get_current_color();

  int remaining = current_health;
  int full_from_led = HEALTH_NUM_LEDS;
  int remainder_value = -1;
  if(is_cache_valid()) {
    full_from_led = cached_full_from_led;
    remainder_value = cached_remainder_value;
  }
  else {
    for(int section = 0; section < 4; section++) {
      int section_count = section_led_count[section];
      
      if(remaining >= HEALTH_BLOCK) {
        remaining -= HEALTH_BLOCK;
        full_from_led -= section_count;
      }
      else {
        // Found the section that contains the led
        int full_in_section = map(remaining, 0, HEALTH_BLOCK, 0, section_count);
        full_from_led -= full_in_section;
  
        if(full_in_section != 0) {
          // Determine partial pixel brightness
          remainder_value = map(remaining, 0, HEALTH_BLOCK, 0, 255 * section_count) - 255 * full_in_section;
        }
        
        break;
      }
    }
  }

  fill_solid(hue_health_leds + full_from_led, HEALTH_NUM_LEDS - full_from_led, fill_color);

  if(remainder_value != -1) {
    hue_health_leds[full_from_led - 1] = fill_color; 
    hue_health_leds[full_from_led - 1].v = remainder_value; 
  }
}

CHSV get_current_color() {
  if(current_health > YELLOW_THRESHOLD) return blue_color;
  else if(current_health > RED_THRESHOLD) return yellow_color;
  else return red_color;
}

bool is_blinked_on = true;

void draw_no_health() {
  EVERY_N_MILLISECONDS(500) {
    blank_healthbar();
    
    if(is_blinked_on){
      // Already set to black
      is_blinked_on = false;
    }
    else {
      hue_health_leds[HEALTH_NUM_LEDS - 1] = red_color;
      is_blinked_on = true;
    }    
  }
}


  //  uint8_t fill_count = map(potRead, 0, 1023, 0, HEALTH_NUM_LEDS + 1);
  //  CHSV fill_color;
  //
  //  if(fill_count > 4) fill_color = blue_color; //CRGB(53, 78, 92);
  //  else if(fill_count > 2) fill_color = yellow_color; //CRGB(100, 99, 6);
  //  else fill_color = red_color; //CRGB(100, 0, 0); //Should this be CRGB(255, 0, 0);??
  //
  //  for(int i = 0; i < fill_count; i++){
  //    hue_health_leds[i] = fill_color;
  //  }
