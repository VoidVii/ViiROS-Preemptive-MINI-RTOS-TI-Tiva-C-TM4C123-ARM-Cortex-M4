/**
 * @file LED.h
 * @brief LED control interface with object-oriented abstraction
 * 
 * This header defines the LED object structure and control interface
 * for managing LEDs in an object-oriented way.
 */

#ifndef LED_H_
#define LED_H_

#include <stdint.h>

/**
 * @brief LED object structure
 * 
 * Encapsulates all properties and methods for an LED:
 * - Write function for hardware control
 * - Optional read function for state reading
 * - Configuration (active high/low)
 * - Current state tracking
 * - Timing parameters for blinking
 */
typedef struct {
    void (*write)(uint8_t value);  /**< Function to write to LED */
    uint8_t (*read)(void);          /**< Optional function to read LED state */
    uint8_t activeHigh;              /**< 1 = HIGH turns ON, 0 = LOW turns ON */
    uint8_t currentState;            /**< Current LED state (0=OFF, 1=ON) */
    uint32_t period;                 /**< Blink period in milliseconds */
    uint32_t last;                   /**< Last update timestamp */
} LED_t;

/**
 * @brief Initialize LED to OFF state
 * @param LED Pointer to LED object to initialize
 */
void LED_init(LED_t *LED);

/**
 * @brief Turn LED ON
 * @param LED Pointer to LED object
 */
void LED_on(LED_t *LED);

/**
 * @brief Turn LED OFF
 * @param LED Pointer to LED object
 */
void LED_off(LED_t *LED);

/**
 * @brief Toggle LED state
 * @param LED Pointer to LED object
 */
void LED_toggle(LED_t *LED);

#endif /* LED_H_ */