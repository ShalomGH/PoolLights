#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>


#define I2C_SLAVE_ADDRESS 0x04 // Address of the slave


byte modes = 0;
byte param = 0;
bool status = 0;
int i = 0;


#define LED_COUNT 5         // num of LEDs
#define LED_DT 3            // LEDs pin
int max_bright = 50;

volatile byte ledMode = 1;

// ---------------СЛУЖЕБНЫЕ ПЕРЕМЕННЫЕ-----------------
int BOTTOM_INDEX = 0;        // светодиод начала отсчёта
int TOP_INDEX = int(LED_COUNT / 2);
int EVENODD = LED_COUNT % 2;
struct CRGB leds[LED_COUNT];
int ledsX[LED_COUNT][3];     //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, MARCH, ETC)


int thisdelay = 20;          //-FX LOOPS DELAY VAR
int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR


int idex = 0;                //-LED INDEX (0 to LED_COUNT-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)
int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR


volatile uint32_t btnTimer;
volatile byte modeCounter;
volatile boolean changeFlag;




void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i].setRGB( cred, cgrn, cblu);
  }
}



void change_mode(byte newmode) {
  thissat = 255;
  switch (newmode) {
    case 8: thisdelay = 60; break;                            //---STRIP RAINBOW FADE
    case 9: thisdelay = 20; thisstep = 10; break;             //---RAINBOW LOOP
    case 10: thisdelay = 20; break;                            //---RANDOM BURST
    case 11: thisdelay = 40; thishue = 0; break;              //---POLICE LIGHTS SOLID
    case 12: thishue = 160; thissat = 50; break;              //---STRIP FLICKER
    case 13: thisdelay = 15; thishue = 0; break;              //---PULSE COLOR BRIGHTNESS
    case 14: thisdelay = 30; thishue = 0; break;              //---PULSE COLOR SATURATION
    case 15: thisdelay = 100; break;                          //---CELL AUTO - RULE 30 (RED)
    case 16: thisdelay = 80; break;                           //---MARCH RANDOM COLORS
    case 17: thisdelay = 80; break;                           //---MARCH RWB COLORS
    case 18: thisdelay = 60; thishue = 95; break;             //---RADIATION SYMBOL
    case 19: thisdelay = 15; break;                           //---NEW RAINBOW LOOP
    case 20: thisdelay = 100; break;                          //---MARCH STRIP NOW CCW
    case 21: thisdelay = 50; break;                           // colorWipe
    case 22: thisdelay = 10; break;                           // rainbowTwinkle
    case 23: thisdelay = 0; break;                            // Sparkle
    case 24: thisdelay = 100; break;                          // Strobe
  }
  bouncedirection = 0;
  one_color_all(0, 0, 0);
}


// receive i2c
void receiveEvent(int howMany) {
 while (0 <Wire.available()) {
    switch(Wire.available()) {
      case 1: status = Wire.read(); break;
      case 2: param = Wire.read(); break;
      case 3: modes = Wire.read(); break;
    }
  }
  Serial.print("mode = ");
  Serial.println(modes);
  Serial.print("params = ");
  Serial.println(param);
  Serial.print("status = ");
  Serial.println(status);
  Serial.println();
  if (modes == 0 && status == 1){
    change_mode(ledMode); // turn on after shutdown with saving last mode
  }
  if (modes > 0 && modes <= 7 && status == 1) {
    ledMode = modes;
  }
  if(modes == 100){
    if (param == 0){
      ledMode--;  // reducing the mode number
    }
    if (param == 1){
      ledMode++;  // increasing the mode number
    }
    change_mode(ledMode);
  }
}


void setup()
{
  Serial.begin(9600);

  Wire.begin(I2C_SLAVE_ADDRESS); // join i2c network
  Wire.onReceive(receiveEvent);

  LEDS.setBrightness(max_bright);
  FastLED.addLeds<NEOPIXEL, LED_DT>(leds, LED_COUNT);

  one_color_all(0, 0, 0);
  LEDS.show();
}


boolean safeDelay(int delTime) {
  uint32_t thisTime;
  thisTime = millis();
  while (true) {
    if (millis() - thisTime >= delTime){
      return true;
      break;
    }
  }
  Serial.println(thisTime);
  thisTime = 0;
  return false;
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
}


void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature / 255.0) * 191);

  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252

  // figure out which third of the spectrum we're in:
  if ( t192 > 0x80) {                    // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if ( t192 > 0x40 ) {            // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}


//---FIND INDEX OF HORIZONAL OPPOSITE LED
int horizontal_index(int i) {
  //-ONLY WORKS WITH INDEX < TOPINDEX
  if (i == BOTTOM_INDEX) {
    return BOTTOM_INDEX;
  }
  if (i == TOP_INDEX && EVENODD == 1) {
    return TOP_INDEX + 1;
  }
  if (i == TOP_INDEX && EVENODD == 0) {
    return TOP_INDEX;
  }
  return LED_COUNT - i;
}

//---FIND INDEX OF ANTIPODAL OPPOSITE LED
int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {
    iN = ( i + TOP_INDEX ) % LED_COUNT;
  }
  return iN;
}


//---FIND ADJACENT INDEX CLOCKWISE
int adjacent_cw(int i) {
  int r;
  if (i < LED_COUNT - 1) {
    r = i + 1;
  }
  else {
    r = 0;
  }
  return r;
}


//---FIND ADJACENT INDEX COUNTER-CLOCKWISE
int adjacent_ccw(int i) {
  int r;
  if (i > 0) {
    r = i - 1;
  }
  else {
    r = LED_COUNT - 1;
  }
  return r;
}


void copy_led_array() {
  for (int i = 0; i < LED_COUNT; i++ ) {
    ledsX[i][0] = leds[i].r;
    ledsX[i][1] = leds[i].g;
    ledsX[i][2] = leds[i].b;
  }
}


void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < LED_COUNT; i++ ) {
    setPixel(i, red, green, blue);
  }
  FastLED.show();
}


void simple_color(){
  switch(ledMode){
    case 2: one_color_all(255, 255, 255); LEDS.show(); break;
    case 3: one_color_all(228, 255, 0); LEDS.show(); break;
    case 4: one_color_all(196, 32, 105); LEDS.show(); break;
    case 5: one_color_all(255, 0, 0); LEDS.show(); break;       //---ALL RED
    case 6: one_color_all(0, 255, 0); LEDS.show(); break;       //---ALL GREEN
    case 7: one_color_all(0, 0, 255); LEDS.show(); break;       //---ALL BLUE
  }
}


void rainbow_fade() {                         //-m2-FADE ALL LEDS THROUGH HSV RAINBOW
  ihue++;
  if (ihue > 255) {
    ihue = 0;
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(ihue, thissat, 255);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void rainbow_loop() {                        //-m3-LOOP HSV RAINBOW
  idex++;
  ihue = ihue + thisstep;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  if (ihue > 255) {
    ihue = 0;
  }
  leds[idex] = CHSV(ihue, thissat, 255);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void random_burst() {                         //-m4-RANDOM INDEX/COLOR
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  leds[idex] = CHSV(ihue, thissat, 255);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void ems_lightsALL() {                  //-m8-EMERGENCY LIGHTS (TWO COLOR SOLID)
  idex++;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  int idexR = idex;
  int idexB = antipodal_index(idexR);
  int thathue = (thishue + 160) % 255;
  leds[idexR] = CHSV(thishue, thissat, 255);
  leds[idexB] = CHSV(thathue, thissat, 255);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void flicker() {                          //-m9-FLICKER EFFECT
  int random_bright = random(0, 255);
  int random_delay = random(10, 100);
  int random_bool = random(0, random_bright);
  if (random_bool < 10) {
    for (int i = 0 ; i < LED_COUNT; i++ ) {
      leds[i] = CHSV(thishue, thissat, random_bright);
    }
    LEDS.show();
    if (safeDelay(random_delay)) return;
  }
}


void pulse_one_color_all() {              //-m10-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR
  if (bouncedirection == 0) {
    ibright++;
    if (ibright >= 255) {
      bouncedirection = 1;
    }
  }
  if (bouncedirection == 1) {
    ibright = ibright - 1;
    if (ibright <= 1) {
      bouncedirection = 0;
    }
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(thishue, thissat, ibright);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void pulse_one_color_all_rev() {          //-m11-PULSE SATURATION ON ALL LEDS TO ONE COLOR
  if (bouncedirection == 0) {
    isat++;
    if (isat >= 255) {
      bouncedirection = 1;
    }
  }
  if (bouncedirection == 1) {
    isat = isat - 1;
    if (isat <= 1) {
      bouncedirection = 0;
    }
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(thishue, isat, 255);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void fade_vertical() {                    //-m12-FADE 'UP' THE LOOP
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  ibright = ibright + 10;
  if (ibright > 255) {
    ibright = 0;
  }
  leds[idexA] = CHSV(thishue, thissat, ibright);
  leds[idexB] = CHSV(thishue, thissat, ibright);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void random_red() {                       //QUICK 'N DIRTY RANDOMIZE TO GET CELL AUTOMATA STARTED
  int temprand;
  for (int i = 0; i < LED_COUNT; i++ ) {
    temprand = random(0, 100);
    if (temprand > 50) {
      leds[i].r = 255;
    }
    if (temprand <= 50) {
      leds[i].r = 0;
    }
    leds[i].b = 0; leds[i].g = 0;
  }
  LEDS.show();
}


void rule30() {                           //-m13-1D CELLULAR AUTOMATA - RULE 30 (RED FOR NOW)
  if (bouncedirection == 0) {
    random_red();
    bouncedirection = 1;
  }
  copy_led_array();
  int iCW;
  int iCCW;
  int y = 100;
  for (int i = 0; i < LED_COUNT; i++ ) {
    iCW = adjacent_cw(i);
    iCCW = adjacent_ccw(i);
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] > y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] > y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {
      leds[i].r = 0;
    }
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void random_march() {                     //-m14-RANDOM MARCH CCW
  copy_led_array();
  int iCCW;
  leds[0] = CHSV(random(0, 255), 255, 255);
  for (int idex = 1; idex < LED_COUNT ; idex++ ) {
    iCCW = adjacent_ccw(idex);
    leds[idex].r = ledsX[iCCW][0];
    leds[idex].g = ledsX[iCCW][1];
    leds[idex].b = ledsX[iCCW][2];
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void rwb_march() {                        //-m15-R,W,B MARCH CCW
  copy_led_array();
  int iCCW;
  idex++;
  if (idex > 2) {
    idex = 0;
  }
  switch (idex) {
    case 0:
      leds[0].r = 255;
      leds[0].g = 0;
      leds[0].b = 0;
      break;
    case 1:
      leds[0].r = 255;
      leds[0].g = 255;
      leds[0].b = 255;
      break;
    case 2:
      leds[0].r = 0;
      leds[0].g = 0;
      leds[0].b = 255;
      break;
  }
  for (int i = 1; i < LED_COUNT; i++ ) {
    iCCW = adjacent_ccw(i);
    leds[i].r = ledsX[iCCW][0];
    leds[i].g = ledsX[iCCW][1];
    leds[i].b = ledsX[iCCW][2];
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void rainbow_vertical() {                 //-m23-RAINBOW 'UP' THE LOOP
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  ihue = ihue + thisstep;
  if (ihue > 255) {
    ihue = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  leds[idexA] = CHSV(ihue, thissat, 255);
  leds[idexB] = CHSV(ihue, thissat, 255);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void rgb_propeller() {                    //-m27-RGB PROPELLER
  idex++;
  int ghue = (thishue + 80) % 255;
  int bhue = (thishue + 160) % 255;
  int N3  = int(LED_COUNT / 3);
  int N12 = int(LED_COUNT / 12);
  for (int i = 0; i < N3; i++ ) {
    int j0 = (idex + i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0 + N3) % LED_COUNT;
    int j2 = (j1 + N3) % LED_COUNT;
    leds[j0] = CHSV(thishue, thissat, 255);
    leds[j1] = CHSV(ghue, thissat, 255);
    leds[j2] = CHSV(bhue, thissat, 255);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void matrix() {                           //-m29-ONE LINE MATRIX
  int rand = random(0, 100);
  if (rand > 90) {
    leds[0] = CHSV(thishue, thissat, 255);
  }
  else {
    leds[0] = CHSV(thishue, thissat, 0);
  }
  copy_led_array();
  for (int i = 1; i < LED_COUNT; i++ ) {
    leds[i].r = ledsX[i - 1][0];
    leds[i].g = ledsX[i - 1][1];
    leds[i].b = ledsX[i - 1][2];
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


void new_rainbow_loop() {                 //-m88-RAINBOW FADE FROM FAST_SPI2
  ihue -= 1;
  fill_rainbow( leds, LED_COUNT, ihue );
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}


//---------------------------------линейный огонь-------------------------------------
void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[LED_COUNT];
  int cooldown;

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < LED_COUNT; i++) {
    cooldown = random(0, ((Cooling * 10) / LED_COUNT) + 2);

    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = LED_COUNT - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if ( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for ( int j = 0; j < LED_COUNT; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  FastLED.show();
  if (safeDelay(SpeedDelay)) return;
}


byte * Wheel(byte WheelPos) {
  static byte c[3];

  if (WheelPos < 85) {
    c[0] = WheelPos * 3;
    c[1] = 255 - WheelPos * 3;
    c[2] = 0;
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    c[0] = 255 - WheelPos * 3;
    c[1] = 0;
    c[2] = WheelPos * 3;
  } else {
    WheelPos -= 170;
    c[0] = 0;
    c[1] = WheelPos * 3;
    c[2] = 255 - WheelPos * 3;
  }

  return c;
}


//-------------------------------newKITT---------------------------------------
void rainbowCycle(int SpeedDelay) {
  byte *c;
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < LED_COUNT; i++) {
      c = Wheel(((i * 256 / LED_COUNT) + j) & 255);
      setPixel(i, *c, *(c + 1), *(c + 2));
    }
    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
  }
}


void loop(){
  if (status == 1){
    switch (ledMode) {
      case  1: ledMode = 24;
      case  2: simple_color(); break;
      case  3: simple_color(); break;
      case  4: simple_color(); break;
      case  5: simple_color(); break;
      case  6: simple_color(); break;
      case  7: simple_color(); break;
      case  8: rainbow_fade(); break;            // плавная смена цветов всей ленты
      case  9: rainbow_loop(); break;            // крутящаяся радуга
      case 10: random_burst(); break;            // случайная смена цветов
      case 11: ems_lightsALL(); break;           // вращается половина красных и половина синих
      case 12: flicker(); break;                 // случайный стробоскоп
      case 13: pulse_one_color_all(); break;     // пульсация одним цветом
      case 14: pulse_one_color_all_rev(); break; // пульсация со сменой цветов
      case 15: fade_vertical(); break;           // плавная смена яркости по вертикали (для кольца)
      case 16: rule30(); break;                  // безумие красных светодиодов
      case 17: random_march(); break;            // безумие случайных цветов
      case 18: rwb_march(); break;               // белый синий красный бегут по кругу (ПАТРИОТИЗМ!)
      case 19: rainbow_vertical(); break;        // радуга в вертикаьной плоскости (кольцо)
      case 20: rgb_propeller(); break;           // RGB пропеллер
      case 21: matrix(); break;                  // зелёненькие бегают по кругу случайно
      case 22: new_rainbow_loop(); break;        // крутая плавная вращающаяся радуга
      case 23: Fire(55, 120, thisdelay); break;  // линейный огонь
      case 24: rainbowCycle(thisdelay); break;   // очень плавная вращающаяся радуга
      case 25: ledMode = 2;
    }
  }
  if (status == 0) {
    one_color_all(0, 0, 0);
    FastLED.show();
  }
  FastLED.show();
}
