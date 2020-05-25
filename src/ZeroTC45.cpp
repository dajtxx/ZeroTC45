#include "ZeroTC45.h"

static void configureGclk(uint8_t gclkId);
static void configureTC(Tc* tc, uint16_t period, boolean oneShot);
static void stopTC(Tc* tc);

static voidFuncPtr tc4Callback;
static voidFuncPtr tc5Callback;

/**
 * Configures the general clock source used to feed the TC4/TC5 timer/counters.
 *
 * Uses GCLK 4 by default. GLCK id 4 is no relationship with TC4,
 * it just seems to be free on Arduinos and similar systems.
 *
 * @param gclkId The ID of the GCLK clock source to use as input for TC4 and TC5. Defaults to 4.
 */
void ZeroTC45::begin(uint8_t gclkId) {
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
void configureTC(Tc* tc, uint16_t period, boolean oneShot) {

    // Disable TC so it can be configured.
	while (tc->COUNT16.STATUS.bit.SYNCBUSY);
    tc->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (tc->COUNT16.STATUS.bit.SYNCBUSY);

    tc->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16        // Use 16-bit counting mode.
                          | TC_CTRLA_WAVEGEN(1)          // Use MFRQ mode so CC0 is 'TOP' and we get an overflow every time CC0 is reached.
                          | TC_CTRLA_RUNSTDBY            // Run when in standby mode.
                          | TC_CTRLA_PRESCALER_DIV1024;  // Divide the input GCLK frequency by 1024.
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
void configureGclk(uint8_t gclkId) {
    // Configure the 32768 Hz oscillator
    SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_ONDEMAND |
                           SYSCTRL_XOSC32K_RUNSTDBY |
                           SYSCTRL_XOSC32K_EN32K |
                           SYSCTRL_XOSC32K_XTALEN | SYSCTRL_XOSC32K_STARTUP(6) |
                           SYSCTRL_XOSC32K_ENABLE;

    //========== GCLK configuration - this is the source clock for the TC.

    // Setup clock provider gclkId with a 32 source divider
    // GCLK_GENDIV_ID(X) specifies which GCLK we are configuring
    // GCLK_GENDIV_DIV(Y) specifies the clock prescalar / divider
    // If GENCTRL.DIVSEL is set (see further below) the divider
    // is 2^(Y+1). If GENCTRL.DIVSEL is 0, the divider is simply Y
    // This register has to be written in a single operation
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(gclkId) | GCLK_GENDIV_DIV(4);
    // GENDIV is not write sync

    // Configure the GCLK module
    // With this configuration the output from this module is 1khz (32khz / 32)
    // This register has to be written in a single operation.
    while (GCLK->STATUS.bit.SYNCBUSY);
    GCLK->GENCTRL.reg = GCLK_GENCTRL_GENEN          // GCLK_GENCTRL_GENEN, enable the specific GCLK module
                      | GCLK_GENCTRL_SRC_XOSC32K    // GCLK_GENCTRL_SRC_XOSC32K, set the source to the XOSC32K
                      | GCLK_GENCTRL_ID(gclkId)     // GCLK_GENCTRL_ID(X), specifies which GCLK is being configured
                      | GCLK_GENCTRL_DIVSEL;        // GCLK_GENCTRL_DIVSEL, specify the divider is 2^(GENDIV.DIV+1)
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
