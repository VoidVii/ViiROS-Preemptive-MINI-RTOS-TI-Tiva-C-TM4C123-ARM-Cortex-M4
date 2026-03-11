/**
 * @file LED.c
 * @brief LED control module with object-oriented abstraction
 * 
 * This module provides high-level control for LEDs using an object-oriented
 * approach. Each LED is represented by an LED_t structure that contains:
 * - A function pointer for hardware-specific write operations
 * - Configuration (activeHigh determines logic level for ON)
 * - Current state tracking
 * 
 * This abstraction allows the same code to work with different LED types
 * (GPIO, I2C, SPI) without modification.
 */

#include "LED.h"

/**
 * @brief Initialize an LED object to OFF state
 * 
 * Ensures the LED starts in a known (OFF) state by calling LED_off().
 * This is important for deterministic system startup.
 * 
 * @param LED_t Pointer to the LED object to initialize
 * 
 * @note The LED's write function must already be configured
 * @note activeHigh setting affects what value is written
 * 
 * @see LED_off() for the actual turn-off logic
 */
void LED_init(LED_t *LED_t)
{
    /* Start with LED turned off */
    LED_off(LED_t);
}

/**
 * @brief Turn an LED on
 * 
 * Sets the LED to ON state, respecting the activeHigh configuration:
 * - If activeHigh = 1, write 1 to turn ON
 * - If activeHigh = 0, write 0 to turn ON
 * 
 * Updates the internal currentState to reflect the new state.
 * 
 * @param LED_t Pointer to the LED object to turn on
 * 
 * @note The write function pointer must be valid
 * @note currentState is updated AFTER writing to hardware
 * 
 * @see LED_off() for turning off
 * @see LED_toggle() for state switching
 */
void LED_on(LED_t *LED_t)
{
    /* Write appropriate value based on activeHigh configuration */
    if(LED_t->activeHigh)
    {
        LED_t->write(1U);   /* Active HIGH: 1 = ON */
    }
    else
    {
        LED_t->write(0U);   /* Active LOW: 0 = ON */
    }
    
    /* Update internal state tracking */
    LED_t->currentState = 1U;
}

/**
 * @brief Turn an LED off
 * 
 * Sets the LED to OFF state, respecting the activeHigh configuration:
 * - If activeHigh = 1, write 0 to turn OFF
 * - If activeHigh = 0, write 1 to turn OFF
 * 
 * Updates the internal currentState to reflect the new state.
 * 
 * @param LED_t Pointer to the LED object to turn off
 * 
 * @note The write function pointer must be valid
 * @note currentState is updated AFTER writing to hardware
 * 
 * @see LED_on() for turning on
 * @see LED_toggle() for state switching
 */
void LED_off(LED_t *LED_t)
{
    /* Write appropriate value based on activeHigh configuration */
    if(LED_t->activeHigh)
    {
        LED_t->write(0U);   /* Active HIGH: 0 = OFF */
    }
    else
    {
        LED_t->write(1U);   /* Active LOW: 1 = OFF */
    }
    
    /* Update internal state tracking */
    LED_t->currentState = 0U;
}

/**
 * @brief Toggle an LED's state (ON ? OFF)
 * 
 * Changes the LED state from ON to OFF or OFF to ON.
 * Uses the internal currentState to determine which action to take.
 * This is more efficient than reading the hardware state.
 * 
 * @param LED_t Pointer to the LED object to toggle
 * 
 * @note Uses currentState for decision, not hardware read
 * @note Equivalent to: if(on) off(); else on();
 * 
 * @see LED_on()
 * @see LED_off()
 */
void LED_toggle(LED_t *LED_t)
{
    /* Toggle based on current state tracking */
    if(LED_t->currentState)
    {
        LED_off(LED_t);     /* Was ON ? turn OFF */
    }
    else
    {
        LED_on(LED_t);      /* Was OFF ? turn ON */
    }
}