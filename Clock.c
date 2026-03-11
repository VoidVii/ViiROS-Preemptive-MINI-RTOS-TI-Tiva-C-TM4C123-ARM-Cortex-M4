/**
 * @file Clock.c
 * @brief System clock configuration module
 * 
 * This module handles the system clock initialization and provides
 * clock-related utility functions. Currently configured for the
 * default 16 MHz system clock after reset.
 * 
 * The TM4C123GH6PM starts up with the main oscillator (MOSC) configured
 * to use the 16 MHz crystal, providing a 16 MHz system clock without
 * any additional configuration.
 */

#include "Clock.h"



/**
 * @brief Calculate SysTick reload value for 1ms period
 * 
 * Computes the value to load into the SysTick reload register to achieve
 * a 1ms interrupt period. The calculation is:
 * 
 * SysTick_Reload = (System_Core_Clock / 1000) - 1
 * 
 * For 16 MHz: (16000000 / 1000) - 1 = 15999
 * 
 * The SysTick timer is a 24-bit counter that counts down from this value
 * to zero at the system clock frequency. When it reaches zero, it triggers
 * an interrupt and automatically reloads.
 * 
 * @return 24-bit reload value for 1ms SysTick period
 * 
 * @note System_Core_Clock must be defined with the correct clock frequency
 * @note Result is guaranteed to fit in 24 bits for reasonable clock speeds
 * @note Subtract 1 because timer counts from RELOAD down to 0 (RELOAD+1 ticks)
 * 
 * @warning This function assumes System_Core_Clock is correctly set
 * @see SysTick_Init() uses this value to configure the timer
 */
uint32_t SysTick_Reload_Value(void)
{
    /**
     * Calculate reload value for 1ms period
     * 
     * Formula: Reload = (Clock_Hz / 1000) - 1
     * 
     * Example with 16 MHz:
     * (16,000,000 / 1000) - 1 = 16,000 - 1 = 15,999
     * 
     * The timer counts 16,000 cycles (from 15999 down to 0) at 16 MHz,
     * taking exactly 1ms (16,000 / 16,000,000 = 0.001 seconds).
     */
    uint32_t SysTick_Reload = (SystemCoreClock / 1000U) - 1U;
    
    return SysTick_Reload;
}