/**
 * @file Debounce_Switch.c
 * @brief Software debouncing implementation for mechanical switches
 * 
 * This module provides debouncing functionality for mechanical switches
 * using a timer-based approach. It filters out the contact bounce that
 * occurs when switches are pressed or released.
 */

#include "Debounce_Switch.h"
#include "SysTick.h"

/**
 * @brief Debounce a mechanical switch and return its stable state
 * 
 * This function implements a classic debouncing algorithm:
 * 1. Read the current raw state of the switch
 * 2. If the raw state changed, reset the debounce timer
 * 3. If the raw state remains stable for the debounce period,
 *    update the stable switch state
 * 
 * @param sw Pointer to the Switch_t object containing:
 *           - read() function to get raw switch state
 *           - lastDebounceTime timestamp of last state change
 *           - lastReading previous raw reading
 *           - switchState current debounced state
 * 
 * @return Current debounced switch state:
 *         - 0 = pressed (active low with pull-up)
 *         - 1 = released (active low with pull-up)
 * 
 * @note Requires SysTick timer initialized for 1ms ticks
 * @note Debounce period is defined in Debounce_Switch.h (typically 20ms)
 * @note Safe for use in periodic tasks (recommended: call every 10-20ms)
 * 
 * @warning The switch's read() function must be fast and non-blocking
 * @warning This function is not reentrant for the same switch object
 * 
 * @see DEBOUNCE_TIME_MS in Debounce_Switch.h
 * @see GetTickCounter() for system time reference
 * 
 * @details Debouncing Algorithm:
 *          - Raw readings may bounce between 0 and 1 for several ms
 *          - Timer starts on first detected change
 *          - If reading stays same for entire debounce period,
 *            the change is accepted as genuine
 *          - This filters out mechanical bounce while maintaining
 *            good response time
 */
uint32_t debounce_switch(Switch_t *sw)
{
    /* Read the current raw state directly from hardware */
    uint8_t currentReading = sw->read();

    /**
     * If raw reading changed, reset the debounce timer
     * 
     * This starts a new debounce period whenever the switch
     * position appears to change. The timer is reset on every
     * bounce to ensure we only accept stable states.
     */
    if (currentReading != sw->lastReading)
    {
        sw->lastDebounceTime = GetTickCounter();
        sw->lastReading = currentReading;
    }

    /**
     * Check if the raw reading has been stable long enough
     * 
     * If the debounce period has elapsed without further changes,
     * we accept this as the genuine switch state.
     */
    if ((GetTickCounter() - sw->lastDebounceTime) >= DEBOUNCE_TIME_MS)
    {
        /**
         * Only update if the state actually changed
         * Prevents unnecessary updates when state is already correct
         */
        if (sw->switchState != currentReading)
        {
            sw->switchState = currentReading;  // Update stable state
        }
    }

    /* Return the current debounced state */
    return sw->switchState;
}