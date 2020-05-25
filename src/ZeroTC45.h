#ifndef ZERO_TC45_H
#define ZERO_TC45_H

#include "Arduino.h"

typedef void(*voidFuncPtr)(void);

class ZeroTC45 {
public:

    /**
     * Create an instance of the ZeroTC45 class.
     *
     * Only one instance of the class should be created
     * per sketch - the TC peripherals and the callback pointers are not shareable.
     */
    ZeroTC45() {};

     /// Initialise the ZeroTC45 object. This must be called before any other methods.
    void begin(uint8_t gclkId = 4);

    /// Set the callback function for the TC4 interrupt.
    void setTc4Callback(voidFuncPtr callback);

    /// Set the callback function for the TC5 interrupt.
    void setTc5Callback(voidFuncPtr callback);

    /// Start TC4 with the given period (seconds), and optionally in one-shot mode
    void startTc4(uint16_t period, boolean oneShot = false);

    /// Start TC5 with the given period (seconds), and optionally in one-shot mode
    void startTc5(uint16_t period, boolean oneShot = false);

    /// Stop TC4.
    void stopTc4();

    /// Stop TC5.
    void stopTc5();
};
#endif
