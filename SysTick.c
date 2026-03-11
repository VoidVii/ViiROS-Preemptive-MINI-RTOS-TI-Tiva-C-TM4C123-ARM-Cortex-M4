/**
 * @file SysTick.c
 * @brief SysTick timer driver for system timekeeping
 * 
 * This module initializes and manages the SysTick timer to provide
 * a 1ms system tick. It maintains a 32-bit counter that increments
 * every millisecond, allowing for accurate time measurements and delays.
 * 
 * The SysTick timer is a 24-bit decrementing counter that generates
 * an interrupt when it reaches zero, automatically reloading from
 * the RELOAD register.
 */

#include "SysTick.h"
#include "TM4C123GH6PM.h"
#include "Clock.h"

/**
 * @brief Initialize the SysTick timer for 1ms interrupts
 * 
 * Configures the SysTick timer to generate an interrupt every 1ms.
 * The reload value is calculated based on the current system clock frequency.
 * 
 * SysTick timer details:
 * - 24-bit decrementing counter
 * - Counts down from RELOAD value to 0
 * - Generates interrupt on reaching 0
 * - Automatically reloads from RELOAD register
 * - Stops when processor is halted for debugging
 * 
 * Register overview:
 * - STCTRL (0xE000E010): Control and status
 * - STRELOAD (0xE000E014): Reload value
 * - STCURRENT (0xE000E018): Current counter value
 * 
 * @note Must be called before using any time-related functions
 * @note System clock must be configured before calling this
 * 
 * @see SysTick_Reload_Value() Calculates reload value based on clock
 * @see GetTickCounter() for reading the current tick count
 */
void SysTick_Init(void)
{
    /* Disable SysTick during configuration to prevent unexpected interrupts */
    //NVIC_ST_CTRL_R = 0U;
    SysTick->CTRL = 0U;
    
    /* Clear current value (write any value to reset) */
    //NVIC_ST_CURRENT_R = 0U;
    SysTick->VAL = 0U;
    
    /**
     * Set reload value for 1ms period
     * 
     * Calculation: RELOAD = (SystemClock / 1000) - 1
     * Example with 16MHz clock: (16,000,000 / 1000) - 1 = 15,999
     * 
     * The SysTick_Reload_Value() function automatically calculates
     * the correct value based on the actual system clock.
     */
    //NVIC_ST_RELOAD_R = SysTick_Reload_Value();
    SysTick->LOAD = SysTick_Reload_Value();
    
    /**
     * Configure SysTick control register:
     * Bit 0: Enable counter (ENABLE)
     * Bit 1: Enable interrupt (INTEN)
     * Bit 2: Clock source - System clock (CLK_SRC)
     */
   // NVIC_ST_CTRL_R = 0x07;  /* Binary 111 = all three bits set */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
    
}

/**
 * @brief 32-bit system tick counter
 * 
 * This static volatile variable holds the number of milliseconds
 * since system startup. It's incremented every 1ms in the SysTick
 * interrupt handler.
 * 
 * - 32-bit width allows for 49.7 days of uptime before overflow
 * - volatile prevents compiler optimizations (changed in interrupt)
 * - static limits scope to this file
 * 
 * Overflow behavior: After reaching 0xFFFFFFFF, the next increment
 * wraps to 0. Subtraction operations handle this gracefully due to
 * unsigned arithmetic modulo 2^32.
 */
static volatile uint32_t TickCounter;

/**
 * @brief SysTick interrupt handler
 * 
 * Called automatically every 1ms when the SysTick timer reaches zero.
 * Increments the global TickCounter to maintain system time.
 * 
 * This handler should be kept as short as possible since it runs
 * at interrupt context. No complex operations should be performed here.
 * 
 * @note Runs at interrupt priority (typically highest)
 * @warning Keep this function minimal to avoid interrupt latency
 */
void SysTick_Handler(void)
{
    /* Increment system tick counter (atomic on 32-bit Cortex-M) */
    TickCounter++;
}

/**
 * @brief Get the current system tick count
 * 
 * Returns the number of milliseconds elapsed since system startup.
 * The counter is incremented every 1ms in the SysTick interrupt.
 * 
 * @return Current tick count in milliseconds
 * 
 * @note Value wraps around after 49.7 days (2^32 ms)
 * @note Subtraction operations are safe across overflow
 * 
 * @see SysTick_Init() Must be called first
 * 
 * Usage example:
 * @code
 * uint32_t start = GetTickCounter();
 * // do something
 * uint32_t elapsed = GetTickCounter() - start;  // Safe even with overflow
 * @endcode
 */
uint32_t GetTickCounter(void)
{
    return TickCounter;
}