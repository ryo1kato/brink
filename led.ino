// PWD ports for LEDs
#include <stdint.h>
#define RED    9
#define GREEN 10
#define BLUE  11


void led_setup() {
  // put your setup code here, to run once:
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, HIGH);
  digitalWrite(BLUE, HIGH);
}

void led_off() {
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
}

void led_rgb(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RED,   r);
  analogWrite(GREEN, g);
  analogWrite(BLUE,  b);
}


