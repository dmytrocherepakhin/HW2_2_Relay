#include <Arduino.h>
#include <cstdint>

#define RELAY_OUT 1
#define RELAY_CONTROL 2

constexpr uint32_t delayMs = 1000;
constexpr uint32_t debounceUs = 10000; // 10 ms
constexpr uint16_t measurementsCount = 10;
volatile uint32_t firstContactEdgeUs = 0;
volatile uint32_t lastContactEdgeUs = 0;
volatile uint8_t lastContactState = HIGH;
volatile uint16_t contactEdgeCount = 0;

volatile bool contactEdgePending = false;
volatile bool measurementActive = false;

uint32_t lastTimeChange = 0;
uint32_t commandTimeUs = 0;

bool relayOn = false;
bool waitingForContact = false;

uint16_t onCounter = 0;
uint16_t offCounter = 0;
uint64_t onTimeSumUs = 0;
uint64_t offTimeSumUs = 0;
uint64_t onBounceTimeSumUs = 0;
uint64_t offBounceTimeSumUs = 0;
uint32_t onBouncePulseSum = 0;
uint32_t offBouncePulseSum = 0;

bool testFinished = false;

void ARDUINO_ISR_ATTR relayContactISR()
{
  if (!measurementActive)
  {
    return;
  }

  const uint32_t nowUs = micros();

  if (contactEdgeCount == 0)
  {
    firstContactEdgeUs = nowUs;
  }

  lastContactEdgeUs = nowUs;
  lastContactState = digitalRead(RELAY_OUT);
  contactEdgeCount++;
  contactEdgePending = true;
}

void setup()
{
  pinMode(RELAY_CONTROL, OUTPUT);
  pinMode(RELAY_OUT, INPUT_PULLUP);
  digitalWrite(RELAY_CONTROL, LOW);
  attachInterrupt(RELAY_OUT, relayContactISR,CHANGE);
  Serial.begin(115200);
  delay(500);
  Serial.println("HW2_2 Setuped");
  Serial.println("Relay delay measurement");
}

void loop()
{
  const uint32_t nowMs = millis();
  const uint32_t nowUs = micros();

  if (!testFinished && !waitingForContact && nowMs - lastTimeChange >= delayMs)
  {
    lastTimeChange = nowMs;
    relayOn = !relayOn;

    noInterrupts();
    measurementActive = false;
    firstContactEdgeUs = 0;
    lastContactEdgeUs = 0;
    contactEdgeCount = 0;
    contactEdgePending = false;
    interrupts();

    commandTimeUs = micros();
    digitalWrite(RELAY_CONTROL, relayOn ? HIGH : LOW);

    noInterrupts();
    measurementActive = true;
    interrupts();

    waitingForContact = true;

    Serial.printf("Command: relay ");
  }

  if (waitingForContact && contactEdgePending)
  {
    uint32_t firstEdgeTimeUs;
    uint32_t lastEdgeTimeUs;
    uint8_t edgeState;
    uint16_t edgeCount;

    noInterrupts();
    firstEdgeTimeUs = firstContactEdgeUs;
    lastEdgeTimeUs = lastContactEdgeUs;
    edgeState = lastContactState;
    edgeCount = contactEdgeCount;
    interrupts();

    if (nowUs - lastEdgeTimeUs >= debounceUs)
    {
      bool edgeIsStable = false;

      noInterrupts();
      if (lastContactEdgeUs == lastEdgeTimeUs && lastContactState == edgeState &&  contactEdgeCount == edgeCount)
      {
        contactEdgePending = false;
        edgeIsStable = true;
      }
      interrupts();

      if (edgeIsStable)
      {
        const uint8_t expectedState =
            relayOn ? LOW : HIGH;

        if (edgeState == expectedState)
        {
          noInterrupts();
          measurementActive = false;
          interrupts();

          waitingForContact = false;
          const uint32_t switchingTimeUs = lastEdgeTimeUs - commandTimeUs;
          const uint32_t bounceDurationUs = lastEdgeTimeUs - firstEdgeTimeUs;
          const uint16_t bouncePulseCount = edgeCount > 1 ? (edgeCount - 1) / 2 : 0;

          if (relayOn)
          {
            onCounter++;
            onTimeSumUs += switchingTimeUs;
            onBounceTimeSumUs += bounceDurationUs;
            onBouncePulseSum += bouncePulseCount;

            Serial.printf("ON  #%u: delay = %.3f ms, bounce = %.3f ms, edges = %u, bounce pulses = %u\n",
                onCounter, switchingTimeUs / 1000.0, bounceDurationUs / 1000.0, edgeCount, bouncePulseCount);
          }

          else
          {
            offCounter++;
            offTimeSumUs += switchingTimeUs;
            offBounceTimeSumUs += bounceDurationUs;
            offBouncePulseSum += bouncePulseCount;

            Serial.printf("OFF #%u: delay = %.3f ms, bounce = %.3f ms, edges = %u, bounce pulses = %u\n",
                offCounter, switchingTimeUs / 1000.0, bounceDurationUs / 1000.0, edgeCount, bouncePulseCount);
          }
        }
      }
    }
  }

  if (!testFinished && onCounter >= measurementsCount && offCounter >= measurementsCount)
  {
    testFinished = true;

    const uint32_t averageOnUs = onTimeSumUs / onCounter;
    const uint32_t averageOffUs = offTimeSumUs / offCounter;
    const uint32_t averageOnBounceUs = onBounceTimeSumUs / onCounter;
    const uint32_t averageOffBounceUs =  offBounceTimeSumUs / offCounter;
    const float averageOnBouncePulses = static_cast<float>(onBouncePulseSum) / onCounter;
    const float averageOffBouncePulses = static_cast<float>(offBouncePulseSum) / offCounter;

    Serial.println("\n------------- RESULTS -------------");
    Serial.printf("Average ON delay : %.3f ms\n", averageOnUs / 1000.0);
    Serial.printf("Average OFF delay: %.3f ms\n", averageOffUs / 1000.0);
    Serial.printf("\nAverage ON bounce :%.3f ms\n", averageOnBounceUs / 1000.0);
    Serial.printf("Average OFF bounce: %.3f ms\n", averageOffBounceUs / 1000.0);
    Serial.printf("Average ON bounce pulses : %.2f\n", averageOnBouncePulses);
    Serial.printf("Average OFF bounce pulses: %.2f\n", averageOffBouncePulses);
  }
}