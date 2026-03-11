/**
 * @file delay_ms.h
 * @brief Simple millisecond delay interface
 * 
 * This header provides a blocking delay function that uses the
 * SysTick timer for millisecond-accurate timing.
 */

#ifndef DELAY_MS_H_
#define DELAY_MS_H_

#include <stdint.h>

/**
 * @brief Blocking delay for specified milliseconds
 * 
 * This function creates a blocking delay by polling the SysTick counter.
 * The CPU is busy-waiting during the delay and cannot perform other tasks.
 * 
 * @param ms Number of milliseconds to delay (0 to 0xFFFFFFFF-1)
 * 
 * @note Requires SysTick timer to be initialized for 1ms interrupts
 * @note Function is blocking - CPU cannot do other work while waiting
 * @warning Do not use in interrupt handlers or time-critical sections
 * @warning Long delays (>> 1000ms) may affect system responsiveness
 * 
 * @see SysTick_Init() Must be called before using this function
 * @see GetTickCounter() Provides the system time reference
 * 
 * Usage example:
 * @code
 * SysTick_Init();           // Initialize timer first
 * LED_on(&red_LED);
 * delay_ms(500);            // Wait 500ms
 * LED_off(&red_LED);
 * @endcode
 */
void delay_ms(uint32_t ms);

#endif /* DELAY_MS_H_ */