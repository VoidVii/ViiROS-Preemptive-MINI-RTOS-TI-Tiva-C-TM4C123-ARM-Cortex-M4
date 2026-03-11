/**
 * @file Clock.h
 * @brief System clock configuration interface
 * 
 * This header defines the interface for system clock initialization
 * and provides access to clock-related constants.
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <stdint.h>

/**
 * @brief System core clock frequency in Hz
 * 
 * This variable is provided by CMSIS and contains the current
 * system clock frequency. It is automatically updated when the
 * clock configuration changes (via SystemCoreClockUpdate()).
 * 
 * Default is 16,000,000 Hz (16 MHz) after reset.
 * 
 * @note Used by SysTick_Reload_Value() to calculate 1ms period
 */
extern uint32_t SystemCoreClock;



/**
 * @brief Calculate SysTick reload value for 1ms period
 * 
 * @return 24-bit reload value for SysTick timer
 * 
 * @note Result = (SystemCoreClock / 1000) - 1
 * @see SysTick_Init() uses this value
 */
uint32_t SysTick_Reload_Value(void);

#endif /* CLOCK_H_ */