/**
 * @file delay_ms.c
 * @brief Simple blocking delay implementation using SysTick timer
 * 
 * This module provides a millisecond delay function that relies on
 * the SysTick timer configured for 1ms interrupts.
 */

#include "delay_ms.h"
#include "SysTick.h"
#include "tm4c123gh6pm.h"
#include <stdint.h>

/**
 * @brief Blocking delay for specified milliseconds
 * 
 * This function creates a blocking delay by polling the SysTick counter.
 * It reads the current tick value at the start and waits until the
 * specified number of milliseconds have elapsed.
 * 
 * @param ms Number of milliseconds to delay (0 to 0xFFFFFFFF-1)
 * 
 * @note Requires SysTick timer to be initialized for 1ms interrupts
 * @note Function is blocking - CPU cannot do other work while waiting
 * @warning Do not use in interrupt handlers or time-critical sections
 * 
 * @details The delay calculation is safe even when the 32-bit tick counter
 *          overflows due to modulo arithmetic properties:
 *          (TickCounter - start) always gives the correct elapsed time
 *          modulo 2^32, even when the counter wraps around.
 * 
 * @see SysTick_Init() Must be called before using this function
 * @see GetTickCounter() Provides the current system time in ms
 */
void delay_ms(uint32_t ms){
    
    /* Lock the starting value */
    uint32_t start = GetTickCounter();
    
    /**
     * Wait loop using modulo-safe subtraction
     * 
     * The expression (GetTickCounter() - start) < ms is safe even after
     * overflow because of the properties of unsigned 32-bit arithmetic:
     * 
     * Example with overflow:
     * - start = 0xFFFFFFF0
     * - GetTickCounter() overflows to 0x00000010
     * - (0x00000010 - 0xFFFFFFF0) = 0x00000020 (correct elapsed ticks!)
     * 
     * This works because subtraction in unsigned arithmetic implicitly
     * performs modulo 2^32 operations.
     */
    while((GetTickCounter() - start) < ms){
        /* Busy-wait - do nothing */
    }
}