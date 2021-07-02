/*
  ZeroTC45 library for Arduino Zero and similar.

  Copyright (c) 2020 David Taylor. All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3.0 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef ZERO_TC45_H
#define ZERO_TC45_H

#include "Arduino.h"

typedef void(*voidFuncPtr)(void);

class ZeroTC45 {

public:
    /// Valid resolutions for the timers. Both timers will use the selected resolution.
    enum Resolution { MILLISECONDS, SECONDS };
    
    /**
     * Create an instance of the ZeroTC45 class.
     *
     * Only one instance of the class should be created
     * per sketch - the TC peripherals and the callback pointers are not shareable.
     */
    ZeroTC45() {};

    /// Initialise the ZeroTC45 object. This must be called before any other methods.
    void begin(uint8_t gclkId = 4);

    /// Initialise the ZeroTC45 object to use the given resolution. Both timers will use this resolution. This must be called before any other methods.
    void begin(Resolution resoluion, uint8_t gclkId = 4);

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

private:
    void configureTC(Tc* tc, uint16_t period, boolean oneShot);
    void configureGclk(uint8_t gclkId);
    
    Resolution resolution;
};
#endif
