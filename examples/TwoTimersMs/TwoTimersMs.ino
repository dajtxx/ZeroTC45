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

static volatile boolean tc5IsrTriggered = false;
static volatile uint32_t tc5IsrMs = 0;
static volatile uint32_t tc5DeltaMs = 0;

void tc4Callback() {
	// Do minimal processing in the callback and just set a flag
	// to indicate the main code path should take some action.
    uint32_t now = millis();
    tc4DeltaMs = now - tc4IsrMs;
    tc4IsrMs = now;
    tc4IsrTriggered = true;
}

void tc5Callback() {
    uint32_t now = millis();
    tc5DeltaMs = now - tc5IsrMs;
    tc5IsrMs = now;
    tc5IsrTriggered = true;
}

void setup() {
    Serial.begin(115200);

	// Wait for a connection from the serial monitor or a terminal emulator.
    while ( ! Serial);

    // Put the timers in milliseconds mode.
    timer.begin(ZeroTC45::MILLISECONDS);

    // Initialise these fields to get a good reading on the first delay
    // for the callbacks. Otherwise the time it took for Serial to start
    // and connect will be added to the first delta.
    tc4IsrMs = tc5IsrMs = millis();

    // Start TC4 and have it call tc4Callback every 1.5 seconds.
    timer.setTc4Callback(tc4Callback);
    timer.startTc4(1500);

    // Start TC5 in one-shot mode, with tc5Callback called 15 seconds from now.
    timer.setTc5Callback(tc5Callback);
    timer.startTc5(15000, true);

    Serial.println("setup done.");
}

static char msg[64];

void loop() {
    if (tc4IsrTriggered) {
        tc4IsrTriggered = false;
        snprintf(msg, sizeof(msg), "%8lu: TC4 ISR triggered with delta %lu", millis(), tc4DeltaMs);
        Serial.println(msg);
    }

    if (tc5IsrTriggered) {
        tc5IsrTriggered = false;
        snprintf(msg, sizeof(msg), "%8lu: TC5 one-shot ISR triggered with delta %lu", millis(), tc5DeltaMs);
        Serial.println(msg);
    }
}