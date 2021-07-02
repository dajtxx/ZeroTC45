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

#include "ZeroTC45.h"

static void configureGclk(uint8_t gclkId);
static void configureTC(Tc* tc, uint16_t period, boolean oneShot);
static void stopTC(Tc* tc);

static voidFuncPtr tc4Callback;
static voidFuncPtr tc5Callback;

/**
 * Initialises the library with a resolution of seconds.
 *
 * Uses GCLK 4 by default. GLCK id 4 is no relationship with TC4,
 * it just seems to be free on Arduinos and similar systems.
 *
 * @param gclkId The ID of the GCLK clock source to use as input for TC4 and TC5. Defaults to 4.
 */
void ZeroTC45::begin(uint8_t gclkId) {
    resolution = SECONDS;
    configureGclk(gclkId);

    // Enable TC4 & TC5 in the power manager.
    PM->APBCMASK.reg |= PM_APBCMASK_TC4;
    PM->APBCMASK.reg |= PM_APBCMASK_TC5;
}

/**
 * Initialises the library with the given resolution.
 *
 * Uses GCLK 4 by default. GLCK id 4 is no relationship with TC4,
 * it just seems to be free on Arduinos and similar systems.
 *
 * @param gclkId The ID of the GCLK clock source to use as input for TC4 and TC5. Defaults to 4.
 */
void ZeroTC45::begin(Resolution resolution, uint8_t gclkId) {
    this->resolution = resolution;
    configureGclk(gclkId);

    // Enable TC4 & TC5 in the power manager.
    PM->APBCMASK.reg |= PM_APBCMASK_TC4;
    PM->APBCMASK.reg |= PM_APBCMASK_TC5;
}

/**
 * This function will be called every time the TC4 counter
 * overflows the period value given to startTc4.
 *
 * If set to NULL the interrupt service routine will
 * not make a callback.
 *
 * @param callback The function to call when the overflow interrupt occurs.
 */
void ZeroTC45::setTc4Callback(voidFuncPtr callback) {
    tc4Callback = callback;
}

/**
 * This function will be called every time the TC5 counter
 * overflows the period value given to startTc5.
 *
 * If set to NULL the interrupt service routine will
 * not make a callback.
 *
 * @param callback The function to call when the overflow interrupt occurs.
 */
void ZeroTC45::setTc5Callback(voidFuncPtr callback) {
    tc5Callback = callback;
}

/**
 * Start TC4 counting. It will cause an interrupt every period seconds
 * and the TC4 callback function given to setTc4Callback will be called
 * each time.
 *
 * If oneShot is true then TC4 will run until the first interrupt and then
 * be stopped. It can be restarted by calling this method again.
 *
 * @param period The amount of time in seconds between calls to the callback function.
 * @param oneShot If true the callback will only be called once.
 */
void ZeroTC45::startTc4(uint16_t period, boolean oneShot) {
    configureTC(TC4, period, oneShot);
}

/**
 * Start TC5 counting. It will cause an interrupt every period seconds
 * and the TC5 callback function given to setTc5Callback will be called
 * each time.
 *
 * If oneShot is true then TC5 will run until the first interrupt and then
 * be stopped. It can be restarted by calling this method again.
 *
 * @param period The amount of time in seconds between calls to the callback function.
 * @param oneShot If true the callback will only be called once.
 */
void ZeroTC45::startTc5(uint16_t period, boolean oneShot) {
    configureTC(TC5, period, oneShot);
}

/**
 * Stop TC4, clear pending interrupts, disable interrupts.
 */
void ZeroTC45::stopTc4() {
    stopTC(TC4);
}

/**
 * Stop TC5, clear pending interrupts, disable interrupts.
 */
void ZeroTC45::stopTc5() {
    stopTC(TC5);
}

/**
 * Stop a TC by giving it a stop command, disabling the TC overflow interrupt, and
 * clearing and disabling the TC-specific interrupt line.
 *
 * @param tc The TC to configure, must be TC4 or TC5.
 */
void stopTC(Tc* tc) {
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);
    tc->COUNT16.CTRLBSET.reg |= TC_CTRLBSET_CMD_STOP;
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);

    while (tc->COUNT16.STATUS.bit.SYNCBUSY);
    tc->COUNT16.INTENCLR.bit.OVF = 1;
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);

    if (tc == TC4) {
        NVIC_ClearPendingIRQ(TC4_IRQn);
        NVIC_DisableIRQ (TC4_IRQn);
    } else if (tc == TC5) {
        NVIC_ClearPendingIRQ(TC5_IRQn);
        NVIC_DisableIRQ (TC5_IRQn);
    }
}

/**
 * Configure the given TC (must be TC4 or TC5) to count in seconds and
 * cause a match interrupt at matchValue seconds.
 *
 * @param tc The TC to configure, must be TC4 or TC5.
 * @param period The amount of time in seconds between overflow interrupts.
 * @param oneShot If true then the TC one-shot mode is enabled.
 */
void ZeroTC45::configureTC(Tc* tc, uint16_t period, boolean oneShot) {

    // Disable TC so it can be configured.
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);
    tc->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);

    uint16_t ctrla = TC_CTRLA_MODE_COUNT16        // Use 16-bit counting mode.
                   | TC_CTRLA_WAVEGEN(1)          // Use MFRQ mode so CC0 is 'TOP' and we get an overflow every time CC0 is reached.
                   | TC_CTRLA_RUNSTDBY;           // Run when in standby mode.

    if (resolution == SECONDS) {
        ctrla |= TC_CTRLA_PRESCALER_DIV1024;      // Divide the input GCLK frequency by 1024.
    }
    
    tc->COUNT16.CTRLA.reg = ctrla;
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);

    // In MFRQ mode, one-shot works and will only generate one overflow interrupt.
    if (oneShot) {
        tc->COUNT16.CTRLBSET.bit.ONESHOT = 1;
        while (tc->COUNT16.STATUS.bit.SYNCBUSY);
    }

    // Set the compare channel 0 value. In MFRQ mode this is used as 'TOP' to see when overflow occurs.
    // The interrupt is generated on the count after the overflow so wait 1 second less than the caller specifies.
    tc->COUNT16.CC[0].reg = period - 1;
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);

    // Enable overflow interrupts.
    tc->COUNT16.INTENSET.bit.OVF = 1;
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);

    // Enable the TC.
    tc->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);

    // Enable the TC interrupt vector
    // Set the priority to max
    if (tc == TC4) {
        NVIC_ClearPendingIRQ(TC4_IRQn);
        NVIC_EnableIRQ (TC4_IRQn);
        NVIC_SetPriority(TC4_IRQn, 0x00);
    } else if (tc == TC5) {
        NVIC_ClearPendingIRQ(TC5_IRQn);
        NVIC_EnableIRQ (TC5_IRQn);
        NVIC_SetPriority(TC5_IRQn, 0x00);
    }
}

/**
 * Configure the given generic clock source to emit a 1kHz signal.
 *
 * The source is driven by the external 32kHz crystal and uses a
 * 32 prescaler to bring it down to 1kHz.
 *
 * It links the genric clock source to TC4/TC5.
 *
 * @param gclkId The ID of the GCLK clock source to use as input for TC4 and TC5.
 */
void ZeroTC45::configureGclk(uint8_t gclkId) {

    if (resolution == SECONDS) {
        // Configure the 32768 Hz oscillator when counting seconds.
        SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_ONDEMAND |
                               SYSCTRL_XOSC32K_RUNSTDBY |
                               SYSCTRL_XOSC32K_EN32K |
                               SYSCTRL_XOSC32K_XTALEN | SYSCTRL_XOSC32K_STARTUP(6) |
                               SYSCTRL_XOSC32K_ENABLE;
    }

    //========== GCLK configuration - this is the source clock for the TC.

    // Setup clock provider gclkId with a 32 source divider
    // GCLK_GENDIV_ID(X) specifies which GCLK we are configuring
    // GCLK_GENDIV_DIV(Y) specifies the clock prescalar / divider
    // If GENCTRL.DIVSEL is set (see further below) the divider
    // is 2^(Y+1). If GENCTRL.DIVSEL is 0, the divider is simply Y
    // This register has to be written in a single operation.
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(gclkId) | GCLK_GENDIV_DIV(4);  // This divisor works for both seconds and milliseconds.
    // GENDIV is not write sync

    // Configure the GCLK module
    // With this configuration the output from this module is 1khz (32khz / 32)
    // This register has to be written in a single operation.
    while (GCLK->STATUS.bit.SYNCBUSY);
    
    uint32_t genctrl = GCLK_GENCTRL_GENEN          // GCLK_GENCTRL_GENEN, enable the specific GCLK module
                      | GCLK_GENCTRL_ID(gclkId)    // GCLK_GENCTRL_ID(X), specifies which GCLK is being configured
                      | GCLK_GENCTRL_DIVSEL;       // GCLK_GENCTRL_DIVSEL, specify the divider is 2^(GENDIV.DIV+1)

    // For seconds, use a 32.768 khz source with dividers 32 and 1024.
    // For milliseconds, use a 32 khz source with a divider of 32.
    genctrl |= (resolution == SECONDS) ? GCLK_GENCTRL_SRC_XOSC32K : GCLK_GENCTRL_SRC_OSCULP32K;
    
    GCLK->GENCTRL.reg = genctrl;
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Set TC4 (shared with TC5) GCLK source to gclkId
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN              // GCLK_CLKCTRL_CLKEN, enable the generic clock
                      | GCLK_CLKCTRL_GEN(gclkId)        // GCLK_CLKCTRL_GEN(X), specifies the GCLK generator source
                      | GCLK_CLKCTRL_ID(GCM_TC4_TC5);   // GCLK_CLKCTRL_ID(X), specifies which TC this GCLK is being fed to
    while (GCLK->STATUS.bit.SYNCBUSY);
}

void TC4_Handler() {
    if (TC4->COUNT16.INTFLAG.reg & TC_INTFLAG_OVF) {
        if (tc4Callback != NULL) {
            tc4Callback();
        }

        TC4->COUNT16.INTFLAG.reg |= TC_INTFLAG_OVF;
    }
}

void TC5_Handler() {
    if (TC5->COUNT16.INTFLAG.reg & TC_INTFLAG_OVF) {
        if (tc5Callback != NULL) {
            tc5Callback();
        }

        TC5->COUNT16.INTFLAG.reg |= TC_INTFLAG_OVF;
    }
}
