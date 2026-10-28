#include <Arduino.h>

#define RELAY_OUT 1
#define RELAY_CONTROL 2

u32_t delayMs = 1000;
u32_t lastTimeChange = 0;
uint16_t counter = 0;

void setup()
{
  pinMode(RELAY_CONTROL, OUTPUT);
  pinMode(RELAY_OUT, INPUT_PULLUP);
  digitalWrite(RELAY_CONTROL, LOW);
  Serial.begin(115200);
  Serial.println("HW2_2 Setuped");
}

void loop()
{
  const uint32_t now = millis();

  if (counter < 10 && now - lastTimeChange >= delayMs)
  {
    lastTimeChange = now;
    digitalWrite(RELAY_CONTROL, !digitalRead(RELAY_CONTROL));
    // delay(100);
    counter++;
    Serial.printf("RELAY_CONTROL: %s    RELAY_OUT: %s\n", digitalRead(RELAY_CONTROL) ? "HIGH" : "LOW", !digitalRead(RELAY_OUT) ? "ON" : "OFF");
  }
}
