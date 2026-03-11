/**
 * @file SysTick.h
 * @brief SysTick timer interface for system timekeeping
 * 
 * This header defines the interface for SysTick timer initialization
 * and system time access.
 */

#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <stdint.h>

/**
 * @brief Initialize SysTick timer for 1ms interrupts
 * 
 * Configures the SysTick timer to generate an interrupt every 1ms.
 * 
 * @note System clock must be configured before calling this
 * @see Clock_Init() must be called first
 */
void SysTick_Init(void);

/**
 * @brief Get current system tick count
 * 
 * Returns the number of milliseconds elapsed since system startup.
 * 
 * @return Current tick count (wraps after 49.7 days)
 * 
 * @note Subtraction operations are safe across overflow
 * @see SysTick_Init() must be called first
 */
uint32_t GetTickCounter(void);

#endif /* SYSTICK_H_ */