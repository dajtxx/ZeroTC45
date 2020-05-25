/*
  Demonstrates the use of the ZeroTC45 library for the Arduino Zero and similar.

  This example code is in the public domain
*/
#include <ZeroTC45.h>

// All vars are marked as static as they should not be visible outside the scope of this file.

// Create the timer instance.
static ZeroTC45 timer;

// Any vars used in the callbacks are marked volatile so the compiler doesn't make assumptions
// about their values. The callbacks run during interrupts so can change the value asynchronously
// to the main code path.
static volatile boolean tc4IsrTriggered = false;
static volatile uint32_t tc4IsrMs = 0;
static volatile uint32_t tc4DeltaMs = 0;

void tc4Callback() {
    // Do minimal processing in the callback and just set a flag
    // to indicate the main code path should take some action.
    uint32_t now = millis();
    tc4DeltaMs = now - tc4IsrMs;
    tc4IsrMs = now;
    tc4IsrTriggered = true;
}

static uint16_t period = 10;

void setup() {
    Serial.begin(115200);

    // Wait for a connection from the serial monitor or a terminal emulator.
    while ( ! Serial);

    timer.begin();

    // Initialise this field to get a good reading on the first delay
    // for the callback. Otherwise the time it took for Serial to start
    // and connect will be added to the first delta.
    tc4IsrMs = millis();

    // Start TC4 and have it call tc4Callback in 10 seconds.
    timer.setTc4Callback(tc4Callback);
    timer.startTc4(period, true);

    Serial.println("setup done.");
}

static char msg[64];

void loop() {
    if (tc4IsrTriggered) {
        tc4IsrTriggered = false;
        snprintf(msg, sizeof(msg), "TC4 ISR triggered with delta %lu", tc4DeltaMs);
        Serial.println(msg);
    }

    // Start another one-shot timer operation if a character is received.
    bool restartTimer = false;
    while (Serial.available()) {
        Serial.read();
        restartTimer = true;
    }

    if (restartTimer) {
        period += 2;
        // Re-initialise to get a proper delta value.
        tc4IsrMs = millis();
        timer.startTc4(period, true);
        Serial.println("One shot operation started.");
    }
}
