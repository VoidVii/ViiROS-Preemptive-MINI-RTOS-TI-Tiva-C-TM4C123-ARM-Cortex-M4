/**************************************************************************//**
 * @file     system_TM4C123.c
 * @brief    CMSIS System Initialization for TI Tiva TM4C123
 ******************************************************************************/

#include "system_TM4C123.h"
#include "TM4C123GH6PM.h"

/* ----------------------------------------------------------------------------
   -- Core Clock
   ---------------------------------------------------------------------------- */

uint32_t SystemCoreClock = 16000000;  /* Default: 16 MHz after reset */

/* ----------------------------------------------------------------------------
   -- SystemInit()
   ---------------------------------------------------------------------------- */

void SystemInit (void) {
    /* TODO: Insert your system initialization code here.
       This includes:
       - Clock source configuration (MOSC/PLL)
       - Flash wait states
       - FPU enable if needed
       - Any other low-level setup
     */
    
    /* At a minimum, enable FPU for Cortex-M4F */
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));  /* set CP10 and CP11 Full Access */
    #endif
}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */

void SystemCoreClockUpdate (void) {
    /* TODO: Read clock configuration registers and calculate actual frequency.
       For now, just keep default.
     */
    SystemCoreClock = 16000000;
}